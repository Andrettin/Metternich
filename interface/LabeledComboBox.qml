import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Column {
	id: labeled_combobox
	spacing: 8 * scale_factor
	
	property string text: ""
	property int initial_index: 0
	property string tooltip: ""
	property var model: []
	readonly property int count: combobox.count
	property int currentIndex: count > 0 ? 0 : -1
	
	signal activated(int index)
	
	SmallText {
		id: label
		text: labeled_combobox.text
		leftPadding: 2 * scale_factor
		
		MouseArea {
			id: text_area
			anchors.fill: parent
			hoverEnabled: true
		}
		
		CustomTooltip {
			text: labeled_combobox.tooltip
			visible: text_area.containsMouse && labeled_combobox.tooltip.length > 0
		}
	}
	
	CustomComboBox {
		id: combobox
		implicitContentWidthPolicy: ComboBox.WidestText
		tooltip: labeled_combobox.tooltip
		model: labeled_combobox.model
		currentIndex: labeled_combobox.currentIndex
		onActivated: function(index) {
			labeled_combobox.activated(index)
		}
	}
}
