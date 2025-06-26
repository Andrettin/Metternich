import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ButtonBase {
	id: button
	width: portrait_image.width + 2 * scale_factor
	height: portrait_image.height + 2 * scale_factor
	radius: circle ? (width * 0.5) : (width * 0.125)
	
	property string portrait_identifier: ""
	property bool circle: false
	property bool transparent: false
	
	Image {
		id: portrait_image
		anchors.verticalCenter: parent.verticalCenter
		//anchors.verticalCenterOffset: button.down ? 1 * scale_factor : 0
		anchors.horizontalCenter: parent.horizontalCenter
		//anchors.horizontalCenterOffset: button.down ? 1 * scale_factor : 0
		source: portrait_identifier.length > 0 ? ("image://portrait/" + portrait_identifier) : "image://empty/"
		fillMode: Image.Pad
		layer.enabled: true
		visible: false
	}
	
	OpacityMask {
		id: opacity_mask_rectangle
        anchors.fill: portrait_image
		width: portrait_image.width
		height: portrait_image.height
		radius: button.radius
        source: portrait_image
		opacity: transparent ? 0.25 : 1
	}
}
