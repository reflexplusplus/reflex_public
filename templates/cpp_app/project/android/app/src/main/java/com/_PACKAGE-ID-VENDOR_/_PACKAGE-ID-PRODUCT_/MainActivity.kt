package com._PACKAGE-ID-VENDOR_._PACKAGE-ID-PRODUCT_

import com.reflex_multimedia.reflex.sdk.ReflexActivity

class MainActivity : ReflexActivity(R.xml.filepaths) {
	companion object {
		init {
			System.loadLibrary("CppLib")
		}
	}
}