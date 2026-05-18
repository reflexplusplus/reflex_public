package com.reflex_multimedia.reflex.sdk

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
	fun PerformRequest(): Int {
		val code = connection.responseCode
		inputStream = if (code >= 200 && code < 300) {
			connection.inputStream
		} else {
			connection.errorStream
		}
		return connection.responseCode
	}

	// Call the next methods only if the retunr from PerformRequest was a good code (200)
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
		if (inputStream == null) return null

		val buffer = ByteArray(maxBytes)
		val length = inputStream!!.read(buffer)
		if (length == -1) { // no more data
			inputStream!!.close()
			return null
		}

//		println("TEMP: ReadResponseBody ${String(buffer)}")
		return buffer.sliceArray(IntRange(0, length - 1))
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