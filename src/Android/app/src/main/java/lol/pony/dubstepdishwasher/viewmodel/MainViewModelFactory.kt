package lol.pony.dubstepdishwasher.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import lol.pony.dubstepdishwasher.model.BLEManager
import lol.pony.dubstepdishwasher.model.core.UserPresets

class MainViewModelFactory(
    private val bleManager: BLEManager,
    private val userPresets: UserPresets
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return MainViewModel(bleManager, userPresets) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}