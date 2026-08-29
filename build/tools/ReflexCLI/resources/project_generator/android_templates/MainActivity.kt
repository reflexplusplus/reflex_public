package [PACKAGE_ID]

import com.reflexplusplus.reflex.sdk.ReflexActivity

class MainActivity : ReflexActivity(R.xml.filepaths) {
	companion object {
		init {
			System.loadLibrary("[NATIVE_LIBRARY]")
		}
	}
}
