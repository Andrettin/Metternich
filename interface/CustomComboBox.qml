import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ComboBox {
	id: combobox
	focusPolicy: Qt.NoFocus
	onClicked: {
		metternich.defines.click_sound.play()
	}
	
	property string tooltip: ""
	
	CustomTooltip {
		text: combobox.tooltip
		visible: combobox.hovered && combobox.tooltip.length > 0
	}
}
