import QtQuick
import QtQuick.Controls

Image {
	id: icon
	source: icon_identifier.length > 0 ? ("image://icon/" + icon_identifier) : "image://empty/"
	
	property string name: ""
	property string icon_identifier: ""
	property string tooltip: ""
	property string description: ""
	
	signal clicked()
	signal entered()
	signal exited()
	
	MouseArea {
		id: icon_mouse_area
		anchors.fill: parent
		hoverEnabled: true
		
		onClicked: {
			icon.clicked()
		}
		
		onEntered: {
			if (name.length > 0) {
				status_text = name
			}
			if (description.length > 0) {
				middle_status_text = description
			}
			icon.entered()
		}
		
		onExited: {
			if (name.length > 0) {
				status_text = ""
			}
			if (description.length > 0) {
				middle_status_text = ""
			}
			icon.exited()
		}
	}
	
	CustomTooltip {
		text: tooltip
		visible: icon_mouse_area.containsMouse && tooltip.length > 0
	}
}
