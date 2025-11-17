package lol.pony.dubstepdishwasher.model.core

data class EditableCurve(
    val points: List<CurvePoint>,
    val loop: Boolean
) {
    init {
        require(points.size >= 2) { "Curve must have at least 2 points" }
        require(points.first().x == 0f) { "First point must be at x=0" }
        require(points.last().x == 1f) { "Last point must be at x=1" }
    }

    fun evaluate(x: Float): Float {
        val xClamp = if (loop) x % 1f else x.coerceIn(0f, 1f)

        // Find the segment containing x
        val sorted = points.sortedBy { it.x }
        val segmentIndex = sorted.indexOfLast { it.x <= xClamp }
        if (segmentIndex == -1) return sorted.first().y // First point
        if (segmentIndex == sorted.lastIndex) return sorted.last().y // Last point

        // Define segment region
        val p0 = sorted[segmentIndex]; val p1 = sorted[segmentIndex + 1]

        // Normalize and apply exponential curve
        val t = (xClamp - p0.x) / (p1.x - p0.x)
        val curved = applyCurve(t, p0) //

        // Lerp y
        return lerp(p0.y, p1.y, curved)
    }
}