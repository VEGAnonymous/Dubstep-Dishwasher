package lol.pony.dubstepdishwasher.ui.components.subcomponents.parallel

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import lol.pony.dubstepdishwasher.model.core.EffectType
import lol.pony.dubstepdishwasher.model.core.ParallelChain
import lol.pony.dubstepdishwasher.model.core.ParallelChainState

@Composable
fun ParallelEditor(
    id: Int,
    parallelChains: Map<Int, ParallelChainState>,
    resourceError: String?,
    onDismissError: () -> Unit,
    onDismiss: () -> Unit,
    onParallelAdd: (Int, ParallelChain, EffectType) -> Unit,
    onParallelRemove: (Int, ParallelChain, Int) -> Unit,
    onParallelReorder: (Int, ParallelChain, Int, Int) -> Unit,
    onParallelBypass: (Int, ParallelChain, Int) -> Unit,
    onParallelSetParam: (Int, ParallelChain, Int, Int, Any) -> Unit
) {
    val chainState = parallelChains[id] ?: ParallelChainState()
    val chainAEffects = chainState.chainAEffects
    val chainBEffects = chainState.chainBEffects

    Box( // Outside window
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xAA000000))
            .pointerInput(Unit) { detectTapGestures { onDismiss() } }
    ) {
        Box( // Window container
            modifier = Modifier
                .align(Alignment.Center)
                .fillMaxWidth(0.9f)
                .fillMaxHeight(0.85f)
                .background(Color(0xFFF5F5F5), shape = RoundedCornerShape(16.dp))
                .pointerInput(Unit) { detectTapGestures { } }
                .padding(vertical = 12.dp, horizontal = 24.dp)
        ) {
            // Window content
            Column(modifier = Modifier.fillMaxSize()) {
                Row(
                    modifier = Modifier.fillMaxSize(),
                    horizontalArrangement = Arrangement.SpaceEvenly
                ) {
                    // Chain A
                    ParallelChainPanel(
                        modifier = Modifier.weight(1f),
                        chain = ParallelChain.A,
                        effects = chainAEffects,
                        id = id,
                        onParallelAdd = onParallelAdd,
                        onParallelRemove = onParallelRemove,
                        onParallelReorder = onParallelReorder,
                        onParallelBypass = onParallelBypass,
                        onParallelSetParam = onParallelSetParam,
                        resourceError = resourceError,
                        onDismissError = onDismissError
                    )

                    VerticalDivider(color = Color.Gray, thickness = 2.dp)

                    // Chain B
                    ParallelChainPanel(
                        modifier = Modifier.weight(1f),
                        chain = ParallelChain.B,
                        effects = chainBEffects,
                        id = id,
                        onParallelAdd = onParallelAdd,
                        onParallelRemove = onParallelRemove,
                        onParallelReorder = onParallelReorder,
                        onParallelBypass = onParallelBypass,
                        onParallelSetParam = onParallelSetParam,
                        resourceError = resourceError,
                        onDismissError = onDismissError
                    )
                }
            }
        }
    }
}