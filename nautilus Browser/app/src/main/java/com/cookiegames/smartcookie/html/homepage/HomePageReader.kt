package com.cookiegames.smartcookie.html.homepage

import android.content.Context
import java.io.BufferedReader
import java.io.InputStreamReader
import javax.inject.Inject

/**
 * The store for the homepage HTML.
 */
class HomePageReader @Inject constructor() {

    fun provideHtml(context: Context): String {
        val inputStream = context.assets.open("homepage.html")
        val bufferedReader = BufferedReader(InputStreamReader(inputStream))
        return bufferedReader.use { it.readText() }
    }

}