import QtQuick
import QtQuick.Controls

Item {
	id: portrait_grid_item
	width: portrait_button.width
	height: portrait_button.height
	focusPolicy: Qt.NoFocus
	
	property string portrait_identifier: ""
	property string tooltip: ""
	readonly property bool hovered: portrait_button.hovered
	
	signal clicked()
	signal entered()
	signal exited()
	
	PortraitButton {
		id: portrait_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		portrait_identifier: portrait_grid_item.portrait_identifier
		tooltip: portrait_grid_item.tooltip
		focusPolicy: Qt.NoFocus
		
		onClicked: {
			portrait_grid_item.clicked()
		}
		
		onHoveredChanged: {
			if (hovered) {
				portrait_grid_item.entered()
			} else {
				portrait_grid_item.exited()
			}
		}
	}
}
