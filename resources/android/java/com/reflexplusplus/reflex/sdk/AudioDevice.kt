package com.reflexplusplus.reflex.sdk

import android.media.AudioDeviceInfo
import java.util.Locale

@Suppress("unused", "MemberVisibilityCanBePrivate") // used from C++
class AudioDevice {
	companion object {
		const val DEFAULT_NUM_CHANNELS = 2
		const val DEFAULT_SAMPLE_RATE = 44100

		private fun capitalize(str: String): String {
			if (str.isEmpty()) return str
			return str.substring(0, 1).uppercase(Locale.getDefault()) + str.substring(1)
		}
	}

	val id: Int
	val type: Int
	val productName: String
	val friendlyName: String
	/// MEMO: can only get default outputs
	val isDefault: Boolean
	val isInput: Boolean
	val numChannels: Int
	val sampleRates: FloatArray

	constructor(adi: AudioDeviceInfo, defaultOutputDevices: List<AudioDeviceInfo>) {
		id = adi.id
		type = adi.type
		productName = "${adi.productName} [${AudioDeviceInfoConverter.typeToStringForGrouping(adi.type)}]"
		friendlyName = capitalize(AudioDeviceInfoConverter.typeToString(adi.type)) +
				(if (adi.address != "") (" (" + adi.address + ")") else "")
		isDefault = defaultOutputDevices.contains(adi)
		isInput = adi.isSource
		numChannels = supportedChannels(adi.channelCounts)
		sampleRates = adi.sampleRates.map { it.toFloat() }.toFloatArray()
	}

	constructor(defaultDeviceName: String, input: Boolean, default: Boolean, sampleRates_: IntArray) {
		id = 0 // oboe::kUnspecified -> let the system root to the "best" device
		type = 0
		productName = defaultDeviceName
		friendlyName = "Default ${if (input) "input" else "output"}"
		isDefault = default
		isInput = input
		numChannels = DEFAULT_NUM_CHANNELS
		sampleRates = sampleRates_.map { it.toFloat() }.toFloatArray()
	}

	private fun supportedChannels(channelCounts: IntArray): Int {
		// Arbitrary supported channel number
		if (channelCounts.isEmpty()) return DEFAULT_NUM_CHANNELS
		return channelCounts.max()
	}
}
