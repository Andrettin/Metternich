import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ButtonBase {
	id: button
	width: (32 * scale_factor) + (use_margins ? (6 * scale_factor) : 0)
	height: (32 * scale_factor) + (use_margins ? (6 * scale_factor) : 0)
	radius: circle ? (width * 0.5) : (width * 0.25)
	
	property string source: ""
	property bool circle: false
	property bool use_opacity_mask: true
	property bool use_margins: true
	
	Item {
		id: icon_image_area
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		width: parent.width - (use_margins ? (1 * scale_factor * 2) : 0) //width inside the border
		height: parent.height - (use_margins ? (1 * scale_factor * 2) : 0)
		layer.enabled: true
		visible: use_opacity_mask === false
		
		Image {
			id: icon_image
			anchors.verticalCenter: parent.verticalCenter
			anchors.verticalCenterOffset: (button.down ? 1 * scale_factor : 0) + Math.floor((icon_image.height - 32 * scale_factor) / 2)
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.horizontalCenterOffset: button.down ? 1 * scale_factor : 0
			source: button.source
			fillMode: Image.Pad
		}
	}
	
	OpacityMask {
		id: opacity_mask_rectangle
        anchors.fill: icon_image_area
		width: icon_image_area.width
		height: icon_image_area.height
		radius: button.radius
        source: icon_image_area
		visible: use_opacity_mask
	}
}
