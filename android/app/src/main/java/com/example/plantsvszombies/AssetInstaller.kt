package com.example.plantsvszombies

import android.content.Context
import android.content.pm.PackageManager
import android.content.res.AssetManager
import android.os.Build
import android.os.SystemClock
import android.util.Log
import java.io.File
import java.io.IOException

internal object AssetInstaller {
    private const val TAG = "LawnAssets"
    private const val LOCK_FILE = "assets.lock"

    // SDL calls getArguments on SDLThread, so copying does not block the UI.
    @Synchronized
    @Suppress("DEPRECATION")
    fun prepare(context: Context): File {
        val packageInfo = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.packageManager.getPackageInfo(context.packageName, PackageManager.PackageInfoFlags.of(0))
        } else {
            context.packageManager.getPackageInfo(context.packageName, 0)
        }
        val revision = "1:${packageInfo.longVersionCode}:${packageInfo.lastUpdateTime}"
        val destination = File(context.filesDir, "lawn-assets")
        val marker = File(destination, LOCK_FILE)
        if (marker.isFile && marker.readText(Charsets.UTF_8) == revision &&
            File(destination, "main.pak").length() > 0) {
            Log.i(TAG, "Using installed assets: ${destination.absolutePath}")
            return destination
        }

        val startedAt = SystemClock.uptimeMillis()
        val staging = File(context.filesDir, "lawn-assets.staging")
        removeDirectory(staging)
        if (!staging.mkdirs()) {
            throw IOException("Cannot create asset staging directory: $staging")
        }
        val sourceRoot = if (context.assets.list("assets").orEmpty().contains("main.pak")) "assets" else ""
        copyTree(context.assets, sourceRoot, staging)
        val mainPak = File(staging, "main.pak")
        if (mainPak.length() <= 0) {
            Log.w(TAG, "main.pak is missing or empty; using loose assets only (fallback)")
        }

        // A partial copy never receives a completion marker. The marker travels
        // with the finished directory when it replaces the previous APK's assets.
        File(staging, LOCK_FILE).outputStream().use { output ->
            output.write(revision.toByteArray(Charsets.UTF_8))
            output.fd.sync()
        }
        removeDirectory(destination)
        if (!staging.renameTo(destination)) {
            throw IOException("Cannot publish installed assets: $destination")
        }
        Log.i(TAG, "Installed assets in ${SystemClock.uptimeMillis() - startedAt} ms: ${destination.absolutePath}")
        return destination
    }

    private fun copyTree(manager: AssetManager, source: String, destination: File) {
        for (name in manager.list(source).orEmpty()) {
            val assetPath = if (source.isEmpty()) name else "$source/$name"
            val target = File(destination, name)
            if (manager.list(assetPath).orEmpty().isNotEmpty()) {
                if (!target.mkdirs()) {
                    throw IOException("Cannot create asset directory: $target")
                }
                copyTree(manager, assetPath, target)
            } else {
                manager.open(assetPath, AssetManager.ACCESS_STREAMING).use { input ->
                    target.outputStream().use { output -> input.copyTo(output, 1024 * 1024) }
                }
            }
        }
    }

    private fun removeDirectory(directory: File) {
        if (directory.exists() && !directory.deleteRecursively()) {
            throw IOException("Cannot remove old asset directory: $directory")
        }
    }
}
