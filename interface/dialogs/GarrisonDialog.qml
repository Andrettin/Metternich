import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: garrison_dialog
	title: "Garrison"
	width: 492 * scale_factor
	height: 492 * scale_factor
	
	readonly property var military_units_model: (selected_province !== null && selected_garrison) ? selected_province.game_data.military_units : []
	
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
		anchors.bottom: military_unit_grid_view.top
		height: 1 * scale_factor
	}
	
	Flickable {
		id: military_unit_grid_view
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
			id: military_unit_grid
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.right: parent.right
			height: rows * 72 * scale_factor
			rows: Math.min(Math.max(6, Math.ceil(military_units_model.length / 2)), military_units_model.length)
			columns: 2
			flow: Grid.TopToBottom
			
			Repeater {
				model: military_units_model
				
				Item {
					width: Math.floor(military_unit_grid.width / 2)
					height: 72 * scale_factor
					clip: true
					
					readonly property var military_unit: model.modelData
					readonly property var military_unit_type: military_unit.type
					
					Image {
						id: military_unit_icon
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						anchors.leftMargin: 8 * scale_factor + (64 * scale_factor - military_unit_icon.width) / 2
						source: "image://icon/" + military_unit.icon.identifier
					}
					
					MouseArea {
						anchors.fill: military_unit_icon
						hoverEnabled: true
						
						onEntered: {
							status_text = military_unit_type.name
						}
						
						onExited: {
							status_text = ""
						}
					}
					
					SmallText {
						id: military_unit_name_label
						text: military_unit.name
						anchors.top: military_unit_icon.top
						anchors.topMargin: 4 * scale_factor
						anchors.left: military_unit_icon.right
						anchors.leftMargin: 4 * scale_factor
					}
				}
			}
		}
	}
	
	Rectangle {
		color: "gray"
		anchors.top: military_unit_grid_view.top
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter
		width: 1 * scale_factor
	}
}
