import QtQuick
import QtQuick.Controls
import "./dialogs"

Flickable {
	id: population_unit_column_flickable
	contentHeight: contentItem.childrenRect.height
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	property var population_units: []
	
	Column {
		id: population_unit_column
		anchors.left: parent.left
		spacing: 8 * scale_factor
		
		Repeater {
			model: population_units
			
			Row {
				spacing: 4 * scale_factor
				
				readonly property var population_unit: model.modelData
				
				Item {
					id: population_unit_icon_area
					anchors.verticalCenter: parent.verticalCenter
					width: 64 * scale_factor
					height: 64 * scale_factor
					
					CustomIconImage {
						id: population_unit_icon
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
						icon_identifier: population_unit.icon.identifier
						name: population_unit.type.name
					}
				}
				
				SmallText {
					anchors.verticalCenter: parent.verticalCenter
					text: number_string(population_unit.size)
					width: 64 * scale_factor
					horizontalAlignment: Text.AlignLeft
				}
				
				SmallText {
					anchors.verticalCenter: parent.verticalCenter
					text: population_unit.employment_type !== null ? population_unit.employment_type.name : ""
					width: 64 * scale_factor
					horizontalAlignment: Text.AlignLeft
				}
			}
		}
	}
}
