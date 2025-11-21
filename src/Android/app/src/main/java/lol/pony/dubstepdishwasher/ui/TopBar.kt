package lol.pony.dubstepdishwasher.ui

import android.os.Build.VERSION.SDK_INT
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import coil.ImageLoader
import coil.compose.rememberAsyncImagePainter
import coil.decode.GifDecoder
import coil.decode.ImageDecoderDecoder
import coil.request.ImageRequest
import coil.size.Size
import com.polidea.rxandroidble3.RxBleDevice
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.GlobalPresetState
import lol.pony.dubstepdishwasher.model.core.MAX_COMPUTE_USAGE
import lol.pony.dubstepdishwasher.model.core.MAX_MEMORY_USAGE
import lol.pony.dubstepdishwasher.R
import lol.pony.dubstepdishwasher.ui.components.PresetManager
import lol.pony.dubstepdishwasher.ui.components.ResourceMeter
import lol.pony.dubstepdishwasher.viewmodel.MainViewModel
import lol.pony.dubstepdishwasher.viewmodel.MainViewModelFactory

@Composable
fun TopBar (
    bleManager: BLEManager,
    device: RxBleDevice?,
    onDisconnect: () -> Unit
) {
    val viewModel: MainViewModel = viewModel(factory = MainViewModelFactory(bleManager))

    val globalState = viewModel.currentGlobalState.collectAsState().value
    val globalPresets = viewModel.globalPresets.collectAsState().value
    var globalPresetContainer by remember { mutableStateOf(GlobalPresetState()) }

    val usage = viewModel.resourceUsage.collectAsState().value
    var showUsageDialog by remember { mutableStateOf(false) }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 14.dp, vertical = 4.dp)
            .height(45.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Row (
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy((-10).dp)
        ) {
            /* FUNNY SPINNING MARE */
            val context = LocalContext.current
            val gifs = listOf(R.drawable.vinyl_1, R.drawable.vinyl_2)
            var gifIndex by remember { mutableIntStateOf(0) }
            val imageLoader = ImageLoader.Builder(context)
                .components {
                    if (SDK_INT >= 28) add(ImageDecoderDecoder.Factory())
                    else add(GifDecoder.Factory())
                }.build()
            Image(
                painter = rememberAsyncImagePainter(
                    ImageRequest.Builder(context).data(data = gifs[gifIndex])
                        .apply(block = { size(Size.ORIGINAL) })
                        .build(), imageLoader = imageLoader
                ),
                contentDescription = null,
                modifier = Modifier.width(50.dp).clickable { gifIndex = (gifIndex + 1) % gifs.size }
            )

            /* LOGO */
            Image(
                painter = painterResource(R.drawable.dubstep_dishwasher_title), // Just text lmao
                contentDescription = "Logo",
                modifier = Modifier
                    .width(150.dp)
                    .fillMaxHeight()
                // .padding(end = 4.dp)
            )
        }

        Spacer(modifier = Modifier.width(32.dp))

        /* PRESET MANAGER */
        PresetManager(
            presets = globalPresets,
            currentData = globalState,
            containerState = globalPresetContainer,
            randomArgs = Unit,
            onStateChange = { globalPresetContainer = it },
            onSave = { name, category -> viewModel.saveGlobalPreset(name, category) },
            onLoad = { preset -> viewModel.loadGlobalPreset(preset.name) },
            onDelete = { name -> viewModel.deleteGlobalPreset(name) },
            onFavorite = { name, favorite -> viewModel.favoriteGlobalPreset(name, favorite) },
            // Interface copy workaround: we know it's literally a data class
            copyContainer = { state, preset -> state.copy(currentPreset = preset) }
        )

        /* RESOURCE MONITOR */
        Row (
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy((-10).dp)
        ) {
            TextButton(
                onClick = { showUsageDialog = true },
                contentPadding = PaddingValues(0.dp),
                modifier = Modifier.size(width = 40.dp, height = 25.dp)
            ) {
                Text(text = "Usage", style = MaterialTheme.typography.labelSmall.copy(color = Color(0xFF000000)))
            }

            ResourceMeter(
                compute = usage.compute,
                memory = usage.memory,
                maxCompute = MAX_COMPUTE_USAGE,
                maxMemory = MAX_MEMORY_USAGE
            )
        }

        Spacer(modifier = Modifier.width(50.dp))

        /* BLE STATUS */
        Row(verticalAlignment = Alignment.CenterVertically) {
            IconButton(
                onClick = {
                    viewModel.clearChain()
                    onDisconnect()
                },
                modifier = Modifier.size(18.dp).padding(top = 3.dp)) {
                Icon(Icons.Filled.Close, contentDescription = "Disconnect")
            }
            Text(
                text = device?.name ?: "Unknown Device",
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.padding(horizontal = 8.dp)
            )
        }
    } // Row

    if (showUsageDialog) {
        AlertDialog(
            onDismissRequest = { showUsageDialog = false },
            title = { Text(text = "MCU Resource Usage (Approx.)", style = MaterialTheme.typography.bodyMedium) },
            text = {
                Column {
                    Text(text = "CPU: ${(usage.compute * 100).toInt()}%", style = MaterialTheme.typography.bodyMedium)
                    Text(text = "RAM (DTCM): ${usage.memory} / 512 kB", style = MaterialTheme.typography.bodyMedium)
                }
            },
            confirmButton = {
                TextButton(onClick = { showUsageDialog = false }) {
                    Text(text = "OK", style = MaterialTheme.typography.bodyMedium)
                }
            }
        )
    }
}