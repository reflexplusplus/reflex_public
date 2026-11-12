package com.reflexplusplus.reflex.sdk

import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL


@Suppress("FunctionName", "LocalVariableName") // Reflex conventions
class HttpConnection(url: String) {
	private val connection: HttpURLConnection = URL(url).openConnection() as HttpURLConnection
	private var method: String = "GET"
	private var inputStream: InputStream? = null

	init {
		connection.useCaches = true
		connection.instanceFollowRedirects = true
		connection.requestMethod = method
		connection.doOutput = false
		System.setProperty("http.keepAlive","false")
	}

	@Suppress("unused") // Used from C++
	fun PutHeader(name: String, value: String) {
		connection.setRequestProperty(name, value)
	}

	@Suppress("unused") // Used from C++
	fun SetBody(body: ByteArray) {
		connection.doOutput = true

		val outputStream = connection.outputStream
		outputStream.write(body)
		outputStream.close()
	}

	@Suppress("unused") // Used from C++
	fun SetMethod(method_: String) {
		method = method_
	}

	@Suppress("unused") // Used from C++
	fun SetTimeout(connectTimeout: Float, readTimeout: Float) {
		connection.connectTimeout = (connectTimeout * 1000).toInt()
		connection.readTimeout = (readTimeout * 1000).toInt()
	}

	// ⚠️ throws RuntimeException; returns the status code.
	@Suppress("unused") // Used from C++
	fun PerformRequest(): Int = connection.responseCode

	@Suppress("unused") // Used from C++
	fun ReadResponseHeaders(): ArrayList<String> {
		val result = arrayListOf<String>()
		val map = connection.headerFields

		for ((key, headerValues) in map) {
			if (key == null) continue

			val it = headerValues.iterator()
			if (it.hasNext()) {
				result.add(key)
				result.add(it.next())
			}
		}

		return result
	}

	@Suppress("unused") // Used from C++
	fun ReadResponseBody(maxBytes: Int): ByteArray? {
		val stream = inputStream ?: OpenResponseBody() ?: return null
		inputStream = stream

		val buffer = ByteArray(maxBytes)
		val length = stream.read(buffer)
		if (length == -1) return null

//		println("TEMP: ReadResponseBody ${String(buffer)}")
		return buffer.sliceArray(IntRange(0, length - 1))
	}

	private fun OpenResponseBody(): InputStream? {
		val code = connection.responseCode
		return if (code in 200..299) connection.inputStream else connection.errorStream
	}

	@Suppress("unused") // Used from C++
	fun Dispose() {
		try {
			inputStream?.close()
			inputStream = null
			connection.disconnect()
		}
		catch (ex: IOException) {
			println("Exception when closing connection: ${ex.message}")
		}
	}
}
