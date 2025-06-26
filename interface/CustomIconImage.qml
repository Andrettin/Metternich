import QtQuick
import QtQuick.Controls

Image {
	id: icon
	source: icon_identifier.length > 0 ? ("image://icon/" + icon_identifier) : "image://empty/"
	
	property string name: ""
	property string icon_identifier: ""
	property string tooltip: ""
	
	MouseArea {
		id: icon_mouse_area
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
		visible: icon_mouse_area.containsMouse && tooltip.length > 0
	}
}
