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

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
			// Create a notification for the foreground service
			createNotificationChannel()

			val notification = createNotification()
			startForeground(1, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK)
		}
		else {
			startService(Intent(this@BackgroundAudioService, BackgroundAudioService::class.java))
		}

		return START_STICKY
    }

//	// MEMO? Doesn't seem called, instead we'll use isServiceRunning in the activity
//	override fun onTaskRemoved(rootIntent: Intent?) {
//		super.onTaskRemoved(rootIntent)
//	}

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            "MusicServiceChannel",
            "Music Service Channel",
            NotificationManager.IMPORTANCE_LOW
        )
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(channel)
    }

    private fun createNotification(): Notification {
        return NotificationCompat.Builder(this, "MusicServiceChannel")
            .setContentTitle("Music Player")
            .setContentText("Playing music...")
            .setSmallIcon(android.R.drawable.ic_media_play)
            .build()
    }
}
