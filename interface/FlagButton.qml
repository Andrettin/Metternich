import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ButtonBase {
	id: button
	width: flag_image.width + 2 * scale_factor
	height: flag_image.height + 2 * scale_factor
	radius: circle ? (width * 0.5) : (width * 0.125)
	
	property string flag: ""
	property bool circle: false
	property bool transparent: false
	
	FlagImage {
		id: flag_image
		anchors.verticalCenter: parent.verticalCenter
		//anchors.verticalCenterOffset: button.down ? 1 * scale_factor : 0
		anchors.horizontalCenter: parent.horizontalCenter
		//anchors.horizontalCenterOffset: button.down ? 1 * scale_factor : 0
		flag: button.flag
		layer.enabled: true
		visible: false
	}
	
	OpacityMask {
		id: opacity_mask_rectangle
        anchors.fill: flag_image
		width: flag_image.width
		height: flag_image.height
		radius: button.radius
        source: flag_image
		opacity: transparent ? 0.25 : 1
	}
}
