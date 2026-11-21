package com.reflexplusplus.reflex.sdk

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.security.keystore.UserNotAuthenticatedException
import android.util.AtomicFile
import java.io.File
import java.io.FileOutputStream
import java.security.KeyStore
import java.security.MessageDigest
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

internal object ReflexSecrets
{
	private const val success = 0
	private const val notFound = 1
	private const val denied = 2
	private const val tooLarge = 3
	private const val failed = 4
	private const val alias = "com.reflexplusplus.Secret.v1"
	private const val maxSize = 64 * 1024 // Matches SecretStorage::kMaxValueSize.

	private fun exists(file: AtomicFile): Boolean
	{
		return file.baseFile.exists() || File(file.baseFile.path + ".bak").exists()
	}

	private fun encryptionKey(create: Boolean): SecretKey?
	{
		val store = KeyStore.getInstance("AndroidKeyStore")
		store.load(null)
		if (store.containsAlias(alias)) return store.getKey(alias, null) as SecretKey
		if (!create) return null
		val generator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore")
		val spec = KeyGenParameterSpec.Builder(alias, KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT).setKeySize(256).setBlockModes(KeyProperties.BLOCK_MODE_GCM).setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE).build()
		generator.init(spec)
		return generator.generateKey()
	}

	//The result starts with SecretStorage::Result, followed by the retrieved bytes on success.
	//Ciphertext lives outside Android backup: the Keystore key cannot be restored on another device.
	@Synchronized
	fun access(context: Context, operation: Int, key: String, value: ByteArray): ByteArray
	{
		try
		{
			val id = key.toByteArray(Charsets.UTF_8)
			val name = MessageDigest.getInstance("SHA-256").digest(id).joinToString("") { "%02x".format(it.toInt() and 255) }
			val directory = File(context.noBackupFilesDir, "reflex_secrets_v1")
			val file = AtomicFile(File(directory, name))
			when (operation)
			{
				0 ->
				{
					if (value.size > maxSize) return byteArrayOf(tooLarge.toByte())
					if (!directory.isDirectory && !directory.mkdirs()) return byteArrayOf(failed.toByte())
					val cipher = Cipher.getInstance("AES/GCM/NoPadding")
					cipher.init(Cipher.ENCRYPT_MODE, encryptionKey(true))
					cipher.updateAAD(id)
					val encrypted = cipher.doFinal(value)
					var stream: FileOutputStream? = null
					try
					{
						stream = file.startWrite()
						stream.write(1)
						stream.write(cipher.iv)
						stream.write(encrypted)
						file.finishWrite(stream)
					}
					catch (error: Exception)
					{
						file.failWrite(stream)
						throw error
					}
					return byteArrayOf(success.toByte())
				}
				1 ->
				{
					if (!exists(file)) return byteArrayOf(notFound.toByte())
					val input = file.openRead()
					val encrypted: ByteArray
					try
					{
						if (input.channel.size() > maxSize + 29) return byteArrayOf(tooLarge.toByte())
						encrypted = input.readBytes()
					}
					finally
					{
						input.close()
					}
					if (encrypted.size < 29 || encrypted[0] != 1.toByte()) return byteArrayOf(failed.toByte())
					val storedKey = encryptionKey(false) ?: return byteArrayOf(failed.toByte())
					val cipher = Cipher.getInstance("AES/GCM/NoPadding")
					cipher.init(Cipher.DECRYPT_MODE, storedKey, GCMParameterSpec(128, encrypted, 1, 12))
					cipher.updateAAD(id)
					val plaintext = cipher.doFinal(encrypted, 13, encrypted.size - 13)
					try
					{
						return byteArrayOf(success.toByte()) + plaintext
					}
					finally
					{
						plaintext.fill(0)
					}
				}
				2 ->
				{
					if (!exists(file)) return byteArrayOf(notFound.toByte())
					file.delete()
					return byteArrayOf((if (exists(file)) failed else success).toByte())
				}
				else -> return byteArrayOf(failed.toByte())
			}
		}
		catch (error: UserNotAuthenticatedException)
		{
			return byteArrayOf(denied.toByte())
		}
		catch (error: SecurityException)
		{
			return byteArrayOf(denied.toByte())
		}
		catch (error: Exception)
		{
			return byteArrayOf(failed.toByte())
		}
		finally
		{
			value.fill(0)
		}
	}
}
