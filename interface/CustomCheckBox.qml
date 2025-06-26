import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

CheckBox {
	id: checkbox
	focusPolicy: Qt.NoFocus
	
	property string tooltip: ""
	
	CustomTooltip {
		text: checkbox.tooltip
		visible: checkbox.hovered && checkbox.tooltip.length > 0
	}
}
