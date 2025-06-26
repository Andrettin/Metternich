import QtQuick
import QtQuick.Controls

Item {
	id: slider
	width: default_width
	height: 16 * scale_factor
	
	readonly property real default_width: 176 * scale_factor
	property int value: 0
	property int secondary_value: value
	property int min_value: 0
	property int max_value: 0
	property bool fill_slider: true
	property bool show_handle: false
	property string tooltip: ""
	property string decrement_button_tooltip: ""
	property string increment_button_tooltip: ""
	
	signal decremented()
	signal incremented()
	signal clicked(int target_value)
	signal entered()
	signal exited()
	
	Rectangle {
		anchors.verticalCenter: parent.verticalCenter
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		height: 16 * scale_factor
		color: "black"
		border.color: "gray"
		border.width: 1 * scale_factor
			
		readonly property int limit_line_space: slider_handle.slider_handle_space
		
		Rectangle {
			id: slider_fill
			anchors.top: parent.top
			anchors.topMargin: 1 * scale_factor
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 1 * scale_factor
			anchors.left: parent.left
			anchors.leftMargin: 1 * scale_factor
			width: Math.floor((parent.width - 2 * scale_factor) * (value - min_value) / (max_value - min_value))
			color: "dimGray"
			visible: fill_slider
		}
		
		SmallText {
			id: value_label
			text: number_string(value) + (secondary_value !== value ? (" (" + number_string(secondary_value) + ")") : "")
			anchors.verticalCenter: parent.verticalCenter
			anchors.horizontalCenter: parent.horizontalCenter
			visible: !show_handle
		}
		
		SliderHandle {
			id: slider_handle
			anchors.verticalCenter: parent.verticalCenter
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.horizontalCenterOffset: slider_handle_space * (value - min_value) / (max_value - min_value) - slider_handle_space / 2
			visible: show_handle
			
			readonly property int slider_handle_space: Math.floor((parent.width + 2 * scale_factor * 2 - decrement_button.width - increment_button.width))
		}
		
		MouseArea {
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.leftMargin: 8 * scale_factor
			anchors.right: parent.right
			anchors.rightMargin: 8 * scale_factor
			hoverEnabled: true

			onClicked: function(mouse) {
				var target_value = Math.round(mouse.x * (max_value - min_value) / width) - min_value
				
				slider.clicked(target_value)
			}
			
			onEntered: {
				slider.entered()
			}
			
			onExited: {
				slider.exited()
			}
			
			CustomTooltip {
				text: tooltip
				visible: parent.containsMouse && tooltip.length > 0
			}
		}
	}
	
	IconButton {
		id: decrement_button
		anchors.verticalCenter: parent.verticalCenter
		anchors.left: parent.left
		width: 16 * scale_factor
		height: 16 * scale_factor
		icon_identifier: "trade_consulate"
		tooltip: decrement_button_tooltip
		use_opacity_mask: false
		
		onReleased: {
			if (value <= min_value) {
				return
			}
			
			slider.decremented()
		}
	}
	
	IconButton {
		id: increment_button
		anchors.verticalCenter: parent.verticalCenter
		anchors.right: parent.right
		width: 16 * scale_factor
		height: 16 * scale_factor
		icon_identifier: "trade_consulate"
		tooltip: increment_button_tooltip
		use_opacity_mask: false
		
		onReleased: {
			if (value >= max_value) {
				return
			}
			
			slider.incremented()
		}
	}
}
