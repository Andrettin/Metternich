import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ComboBox {
	id: combobox
	focusPolicy: Qt.NoFocus
	
	property string tooltip: ""
	
	CustomTooltip {
		text: combobox.tooltip
		visible: combobox.hovered && combobox.tooltip.length > 0
	}
}
