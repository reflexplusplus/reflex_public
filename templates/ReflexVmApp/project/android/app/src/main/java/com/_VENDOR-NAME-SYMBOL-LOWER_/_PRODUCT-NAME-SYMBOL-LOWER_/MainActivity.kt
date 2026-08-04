package com._VENDOR-NAME-SYMBOL-LOWER_._PRODUCT-NAME-SYMBOL-LOWER_

import com.reflex_multimedia.reflex.sdk.ReflexActivity

class MainActivity : ReflexActivity(R.xml.filepaths) {
	companion object {
		init {
			System.loadLibrary("CppLib")
		}
	}
}