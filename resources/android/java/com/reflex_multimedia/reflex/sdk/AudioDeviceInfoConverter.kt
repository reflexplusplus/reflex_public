package com.reflex_multimedia.reflex.sdk;
/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import android.media.AudioDeviceInfo

internal object AudioDeviceInfoConverter {

    /**
     * Converts an [AudioDeviceInfo] object into a human-readable representation.
     */
    fun toString(adi: AudioDeviceInfo): String {
        return buildString {
            append("Id: ")
            append(adi.id)

            append("\nProduct name: ")
            append(adi.productName)

            append("\nType: ")
            append(typeToString(adi.type))

            append("\nIs source: ")
            append(if (adi.isSource) "Yes" else "No")

            append("\nIs sink: ")
            append(if (adi.isSink) "Yes" else "No")

            append("\nChannel counts: ")
            append(adi.channelCounts.intArrayToString())

            append("\nChannel masks: ")
            append(adi.channelMasks.intArrayToString())

            append("\nChannel index masks: ")
            append(adi.channelIndexMasks.intArrayToString())

            append("\nEncodings: ")
            append(adi.encodings.intArrayToString())

            append("\nSample Rates: ")
            append(adi.sampleRates.intArrayToString())
        }
    }

    /**
     * Converts the value from [AudioDeviceInfo.getType] into a human readable string.
     */
    fun typeToString(type: Int): String {
        return when (type) {
            AudioDeviceInfo.TYPE_BUILTIN_EARPIECE -> "built-in earphonespeaker"

            AudioDeviceInfo.TYPE_BUILTIN_SPEAKER -> "built-in speaker"
            AudioDeviceInfo.TYPE_WIRED_HEADSET -> "wired headset"
            AudioDeviceInfo.TYPE_WIRED_HEADPHONES -> "wired headphones"
            AudioDeviceInfo.TYPE_LINE_ANALOG -> "line analog"
            AudioDeviceInfo.TYPE_LINE_DIGITAL -> "line digital"
            AudioDeviceInfo.TYPE_BLUETOOTH_SCO -> "BT for telephony"
            AudioDeviceInfo.TYPE_BLUETOOTH_A2DP -> "BT for audio"
            AudioDeviceInfo.TYPE_HDMI -> "HDMI"
            AudioDeviceInfo.TYPE_HDMI_ARC -> "HDMI audio return channel"
            AudioDeviceInfo.TYPE_USB_DEVICE -> "USB device"
            AudioDeviceInfo.TYPE_USB_ACCESSORY -> "USB accessory"
            AudioDeviceInfo.TYPE_DOCK -> "dock"
            AudioDeviceInfo.TYPE_FM -> "FM"
            AudioDeviceInfo.TYPE_BUILTIN_MIC -> "built-in microphone"
            AudioDeviceInfo.TYPE_FM_TUNER -> "FM tuner"
            AudioDeviceInfo.TYPE_TV_TUNER -> "TV tuner"
            AudioDeviceInfo.TYPE_TELEPHONY -> "telephony"
            AudioDeviceInfo.TYPE_AUX_LINE -> "Aux line"
            AudioDeviceInfo.TYPE_IP -> "IP"
            AudioDeviceInfo.TYPE_BUS -> "bus"
            AudioDeviceInfo.TYPE_USB_HEADSET -> "USB headset"
            AudioDeviceInfo.TYPE_HEARING_AID -> "hearing aid"
            AudioDeviceInfo.TYPE_BUILTIN_SPEAKER_SAFE -> "built-in speaker(safe)"
            AudioDeviceInfo.TYPE_REMOTE_SUBMIX -> "remote submix"
            AudioDeviceInfo.TYPE_BLE_HEADSET -> "BLE headset"
            AudioDeviceInfo.TYPE_BLE_SPEAKER -> "BLE speaker"
            28 -> "Echo reference"
            29 -> "HDMI enhanced audio return channel"
            AudioDeviceInfo.TYPE_BLE_BROADCAST -> "BLE broadcast group"
            AudioDeviceInfo.TYPE_DOCK_ANALOG -> "analog dock"
            AudioDeviceInfo.TYPE_UNKNOWN -> "unknown type"
            else -> "unknown $type"
		}
    }

    /**
     * Groups some system devices together so they return the same name.
     */
    fun typeToStringForGrouping(type: Int): String {
        return when (type) {
            AudioDeviceInfo.TYPE_BUILTIN_EARPIECE,
                    AudioDeviceInfo.TYPE_TELEPHONY ->
            "device telephony"

            AudioDeviceInfo.TYPE_BUILTIN_MIC,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER ->
            "on-device"

            else -> typeToString(type)
        }
    }

    private fun IntArray.intArrayToString(): String {
        return joinToString(separator = " ")
    }
}