package lol.pony.dubstepdishwasher.model.core

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.dataStore
import kotlinx.serialization.json.Json
import androidx.datastore.core.Serializer
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import java.io.InputStream
import java.io.OutputStream
import kotlinx.serialization.Serializable

@Serializable
data class GlobalPresetStore(
    val presets: List<GlobalPreset> = defaultGlobalPresets()
)

object GlobalPresetSerializer : Serializer<GlobalPresetStore> {

    override val defaultValue: GlobalPresetStore = GlobalPresetStore()

    override suspend fun readFrom(input: InputStream): GlobalPresetStore =
        try {
            Json.decodeFromString(
                GlobalPresetStore.serializer(),
                input.readBytes().decodeToString()
            )
        } catch (e: Exception) {
            e.printStackTrace()
            defaultValue
        }

    override suspend fun writeTo(t: GlobalPresetStore, output: OutputStream) {
        output.write(
            Json.encodeToString(GlobalPresetStore.serializer(), t)
                .encodeToByteArray()
        )
    }
}

val Context.globalPresetDataStore: DataStore<GlobalPresetStore> by dataStore(
    fileName = "global_presets.json",
    serializer = GlobalPresetSerializer
)

class UserPresets(private val context: Context) {

    val presetsFlow: Flow<List<GlobalPreset>> =
        context.globalPresetDataStore.data.map { it.presets }

    suspend fun savePresets(newList: List<GlobalPreset>) {
        context.globalPresetDataStore.updateData { current ->
            current.copy(presets = newList)
        }
    }
}

