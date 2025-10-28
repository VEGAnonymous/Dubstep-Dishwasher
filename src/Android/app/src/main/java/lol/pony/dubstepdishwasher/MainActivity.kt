package lol.pony.dubstepdishwasher

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

import lol.pony.dubstepdishwasher.ui.*
import lol.pony.dubstepdishwasher.ui.theme.DubstepDishwasherTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            DubstepDishwasherTheme {
                Row(Modifier.fillMaxSize()) {
                    FXPanel(
                        modifier = Modifier
                            .fillMaxHeight()
                            .width(280.dp)
                            .padding(16.dp)
                    )
                }
            }
        }
    }
}