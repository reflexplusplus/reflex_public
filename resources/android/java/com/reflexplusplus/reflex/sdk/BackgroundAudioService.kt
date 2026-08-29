package com.reflexplusplus.reflex.sdk

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import androidx.core.app.NotificationCompat

// MEMO: the service is only here so that Android won't kill our process. We don't actually need to
// process the audio from that service. Since it runs on a different thread, it saves some logic to
// just keep outputting the audio on the main thread if it works as is.
class BackgroundAudioService : Service() {

	override fun onBind(intent: Intent?) = null

	override fun onStartCommand(intent: Intent?, flags: Int, startId: Int) = START_NOT_STICKY

	override fun onCreate() {
		super.onCreate()

		createNotificationChannel()

		val kNotificationId = 1
		val notification = createNotification()

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			startForeground(kNotificationId, notification,ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK)
		} else {
			startForeground(kNotificationId, notification)
		}
	}

	private fun createNotificationChannel() {
		val channel = NotificationChannel(
			"AudioServiceChannel",
			"Audio Service Channel",
			NotificationManager.IMPORTANCE_LOW
		)
		val manager = getSystemService(NotificationManager::class.java)
		manager.createNotificationChannel(channel)
	}

	private fun createNotification(): Notification {
		return NotificationCompat.Builder(this, "AudioServiceChannel")
			.setContentTitle("Audio Player")
			.setContentText("Playing audio…")
			.setSmallIcon(android.R.drawable.ic_media_play)
			.build()
	}
}
