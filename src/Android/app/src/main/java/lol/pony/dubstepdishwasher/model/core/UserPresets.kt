package lol.pony.dubstepdishwasher.model.core

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.core.Serializer
import androidx.datastore.dataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.InputStream
import java.io.OutputStream

@Serializable
data class UserPresetStore(
    val globalPresets: List<GlobalPreset> = defaultGlobalPresets(),
    val curvePresets: List<CurvePreset> = defaultCurvePresets()
)

object UserPresetSerializer : Serializer<UserPresetStore> {

    override val defaultValue: UserPresetStore = UserPresetStore()

    override suspend fun readFrom(input: InputStream): UserPresetStore =
        try {
            Json.decodeFromString(
                UserPresetStore.serializer(),
                input.readBytes().decodeToString()
            )
        } catch (e: Exception) {
            e.printStackTrace()
            defaultValue
        }

    override suspend fun writeTo(t: UserPresetStore, output: OutputStream) {
        output.write(
            Json.encodeToString(UserPresetStore.serializer(), t)
                .encodeToByteArray()
        )
    }
}

val Context.userPresetDataStore: DataStore<UserPresetStore> by dataStore(
    fileName = "user_presets.json",
    serializer = UserPresetSerializer
)

class UserPresets(private val context: Context) {

    val userPresetsFlow: Flow<UserPresetStore> =
        context.userPresetDataStore.data.map { it }

    suspend fun savePresets(
        newGlobalPresets: List<GlobalPreset>,
        newCurvePresets: List<CurvePreset>
        ) {
        context.userPresetDataStore.updateData { current ->
            current.copy(
                globalPresets = newGlobalPresets,
                curvePresets = newCurvePresets
                )
        }
    }
}

