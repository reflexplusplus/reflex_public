package com.reflexplusplus.reflex.sdk

import android.os.Bundle
import android.view.GestureDetector
import android.view.MotionEvent

open class ReflexActivityWithOsGestures(private val filepathsResourceId: Int) : ReflexActivity(filepathsResourceId) {
	private lateinit var gestureDetector: GestureDetector
	private var isLongPress = false

	// Touch events for native processing
	private external fun onTouchBegin()
	private external fun onTouchDown(x: Float, y: Float, rightMouseButton: Boolean = false, doubleClick: Boolean = false)
	private external fun onTouchMove(x: Float, y: Float)  // Sent only between a onTouchDown and onTouchUp
	private external fun onTouchUp(x: Float, y: Float, rightMouseButton: Boolean = false)
	private external fun onTouchScroll(x: Float, y: Float, vx: Float, vy: Float) // Sent without a prior onTouchDown
	private external fun onTouchFling(x: Float, y: Float, vx: Float, vy: Float) // Sent without a prior onTouchDown

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)

		// Scaling is not supported for now, but easy to implement: https://developer.android.com/develop/ui/views/touch-and-input/gestures/scale
		// TODO: mouse support can be implemented: https://developer.android.com/games/playgames/input-mouse
		gestureDetector = GestureDetector(this, object : GestureDetector.OnGestureListener {
			override fun onDown(event: MotionEvent): Boolean {
				// Triggered every time
				onTouchBegin()
				return true
			}

			override fun onShowPress(event: MotionEvent) { /* Very short delay, could be used as a mouse down */ }

			override fun onSingleTapUp(event: MotionEvent): Boolean {
				onTouchDown(event.x, event.y)
				onTouchUp(event.x, event.y)
				return true
			}

			override fun onScroll(event: MotionEvent?, event1: MotionEvent, vx: Float, vy: Float): Boolean {
				if (!isLongPress) {
					onTouchScroll(event?.x ?: 0.0f, event?.y ?: 0.0f, vx / intScreenDensity, vy / intScreenDensity)
				}
				return true
			}

			override fun onLongPress(event: MotionEvent) {
				isLongPress = true
				onTouchDown(event.x, event.y)
			}

			override fun onFling(event: MotionEvent?, event1: MotionEvent, vx: Float, vy: Float): Boolean {
				if (!isLongPress) {
					onTouchFling(event?.x ?: 0.0f, event?.y ?: 0.0f, vx / intScreenDensity, vy / intScreenDensity)
				}
				return true
			}
		})

		gestureDetector.setOnDoubleTapListener(object : GestureDetector.OnDoubleTapListener {
			override fun onSingleTapConfirmed(event: MotionEvent): Boolean {
				// onSingleTapUp is more responsive
//				onTouchDown(event.x, event.y)
//				onTouchUp(event.x, event.y)
				return true
			}

			override fun onDoubleTap(event: MotionEvent): Boolean {
				// We send the tap upon mouse up
				onTouchDown(event.x, event.y, doubleClick = true)
				onTouchUp(event.x, event.y)
				return true
			}

			override fun onDoubleTapEvent(motionEvent: MotionEvent): Boolean { /* Will happen many times for one double click */ return true }
		})
	}

	override fun onTouchEvent(event: MotionEvent?): Boolean {
		if (event != null) {
			if (isLongPress) {
				if (event.action == MotionEvent.ACTION_POINTER_UP || event.action == MotionEvent.ACTION_UP) {
					isLongPress = false
					onTouchUp(event.x, event.y)
				}

				if (event.action == MotionEvent.ACTION_MOVE) {
					onTouchMove(event.x, event.y)
				}
			}

			if (gestureDetector.onTouchEvent(event)) {
				return true
			}
		}

		return super.onTouchEvent(event)
	}

	// Quick hack before we have the proper touch API
//	override fun onTouchEvent(event: MotionEvent?): Boolean {
//		if (event != null) {
//			if (event.action == MotionEvent.ACTION_POINTER_DOWN || event.action == MotionEvent.ACTION_DOWN) {
//				val now = System.currentTimeMillis()
//				val doubleClick = now - lastClickTime < 300
//				lastClickTime = now
//				onTouchDown(event.x, event.y, false, doubleClick)
//			}
//
//			if (event.action == MotionEvent.ACTION_POINTER_UP || event.action == MotionEvent.ACTION_UP) {
//				onTouchUp(event.x, event.y)
//			}
//
//			if (event.action == MotionEvent.ACTION_MOVE) {
//				onTouchMove(event.x, event.y)
//			}
//		}
//
//		return super.onTouchEvent(event)
//	}
}
