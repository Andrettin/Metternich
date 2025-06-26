import QtQuick
import QtQuick.Controls

Grid {
	id: portrait_grid
	width: columns * (portrait_width + spacing) - spacing
	columns: Math.min(Math.floor((parent.width - min_spacing) / (portrait_width + min_spacing)), entries.length)
	spacing: columns === entries.length ? min_spacing : Math.max(min_spacing, Math.floor((parent.width - columns * portrait_width) / (columns + 1)))
	visible: entries.length > 0
	
	property var entries: []
	property Component delegate
	readonly property int portrait_width: 64 * scale_factor + 2 * scale_factor
	readonly property int min_spacing: 16 * scale_factor
	
	Repeater {
		model: portrait_grid.entries
		delegate: portrait_grid.delegate
	}
}
