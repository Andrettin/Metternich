import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

RoundButton {
	id: button
	radius: Math.min(width, height) * 0.25
	clip: true
	
	Rectangle {
		anchors.fill: parent
		color: Universal.background
		radius: parent.radius
		z: -1
	}
	
	property string tooltip: ""
	
	CustomTooltip {
		text: button.tooltip
		visible: button.hovered && button.tooltip.length > 0
	}
}
