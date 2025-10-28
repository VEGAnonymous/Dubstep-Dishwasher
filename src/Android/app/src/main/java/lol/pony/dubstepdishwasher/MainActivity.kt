package lol.pony.dubstepdishwasher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge

import lol.pony.dubstepdishwasher.ui.*
import lol.pony.dubstepdishwasher.ui.theme.DubstepDishwasherTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            DubstepDishwasherTheme {
                FXPanel()
            }
        }
    }
}