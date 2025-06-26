import QtQuick
import QtQuick.Controls

Item {
	id: industry_counter
	width: 64 * scale_factor
	height: 48 * scale_factor
	
	property string name: ""
	property string icon_identifier: ""
	property int count: 0
	property string tooltip: ""
	
	Image {
		id: icon_image
		anchors.top: parent.top
		anchors.horizontalCenter: parent.horizontalCenter
		source: "image://icon/" + icon_identifier
	}
	
	SmallText {
		text: number_string(count)
		anchors.top: icon_image.bottom
		anchors.topMargin: 4 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
	}
	
	MouseArea {
		id: industry_counter_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		onEntered: {
			status_text = name
		}
		
		onExited: {
			status_text = ""
		}
	}
	
	CustomTooltip {
		text: tooltip
		visible: industry_counter_mouse_area.containsMouse && tooltip.length > 0
	}
}
