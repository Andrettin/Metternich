import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: garrison_dialog
	width: 492 * scale_factor
	height: 492 * scale_factor
	
	property var units_model: []
	
	IconButton {
		id: close_button
		anchors.top: parent.top
		anchors.topMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		icon_identifier: "skull"
		
		onClicked: {
			garrison_dialog.close()
		}
		
		onHoveredChanged: {
			if (hovered) {
				status_text = "Close"
			} else {
				status_text = ""
			}
		}
	}
	
	Rectangle {
		color: "gray"
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: unit_grid_view.top
		height: 1 * scale_factor
	}
	
	Flickable {
		id: unit_grid_view
		anchors.top: title_item.bottom
		anchors.topMargin: 24 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 1 * scale_factor
		anchors.left: parent.left
		anchors.right: parent.right
		contentHeight: contentItem.childrenRect.height
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		
		Grid {
			id: unit_grid
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.right: parent.right
			height: rows * 72 * scale_factor
			rows: Math.min(Math.max(6, Math.ceil(units_model.length / 2)), units_model.length)
			columns: 2
			flow: Grid.TopToBottom
			
			Repeater {
				model: units_model
				
				Item {
					width: Math.floor(unit_grid.width / 2)
					height: 72 * scale_factor
					clip: true
					
					readonly property var unit: model.modelData
					readonly property var unit_type: unit.type
					
					Image {
						id: unit_icon
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						anchors.leftMargin: 8 * scale_factor + (64 * scale_factor - unit_icon.width) / 2
						source: "image://icon/" + unit.icon.identifier
					}
					
					MouseArea {
						anchors.fill: unit_icon
						hoverEnabled: true
						
						onEntered: {
							status_text = unit_type.name
						}
						
						onExited: {
							status_text = ""
						}
					}
					
					SmallText {
						id: unit_name_label
						text: unit.name
						anchors.top: unit_icon.top
						anchors.topMargin: 4 * scale_factor
						anchors.left: unit_icon.right
						anchors.leftMargin: 4 * scale_factor
					}
				}
			}
		}
	}
	
	Rectangle {
		color: "gray"
		anchors.top: unit_grid_view.top
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter
		width: 1 * scale_factor
	}
}
