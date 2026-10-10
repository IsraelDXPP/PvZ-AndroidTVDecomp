package com.example.plantsvszombies

import org.libsdl.app.SDLActivity

class MainActivity : SDLActivity() {
    override fun sendCommand(command: Int, data: Any?): Boolean {
        // Keep Android system bars hidden independently of native SDL window flags.
        val commandData = if (command == COMMAND_CHANGE_WINDOW_STYLE) 1 else data
        return super.sendCommand(command, commandData)
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            SDLActivity.setWindowStyle(true)
        }
    }

    override fun getLibraries(): Array<String> {
        return arrayOf("SDL3", "main")
    }

    override fun getArguments(): Array<String> {
        val assetDirectory = AssetInstaller.prepare(this)
        return arrayOf("--assets", assetDirectory.absolutePath)
    }
}
