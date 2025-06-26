import QtQuick
import QtQuick.Controls

Rectangle {
	id: infopanel
	color: interface_background_color
	width: 64 * scale_factor
	clip: true
	
	PanelTiledBackground {
	}
	
	Rectangle {
		color: "gray"
		anchors.top: parent.top
		anchors.topMargin: 15 * scale_factor
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15 * scale_factor
		anchors.right: parent.right
		width: 1 * scale_factor
	}
	
	IndustryCounter {
		id: labor_counter
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.topMargin: 96 * scale_factor
		name: "Labor"
		icon_identifier: "labor"
		count: (country_game_data.commodity_outputs && country_game_data.commodity_inputs) ? (country_game_data.get_commodity_output("labor") - country_game_data.get_commodity_input("labor")) : 0 //the conditional is there to make the counter be updated when the labor output or input changes
	}
	
	ListView {
		id: population_unit_list
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.top: labor_counter.bottom
		anchors.topMargin: 8 * scale_factor
		anchors.bottom: back_button.top
		anchors.bottomMargin: 8 * scale_factor
		boundsBehavior: Flickable.StopAtBounds
		spacing: 8 * scale_factor
		clip: true
		model: country_game_data.population.type_counts
		delegate: IndustryCounter {
			name: population_type.name
			icon_identifier: country_game_data.get_population_type_small_icon(population_type).identifier
			count: population_count
			tooltip: modifier_string.length > 0 ? format_text(small_text(modifier_string)) : ""
			
			readonly property var population_type: model.modelData.key
			readonly property int population_count: model.modelData.value
			readonly property string modifier_string: population_type.get_country_modifier_string(country)
		}
	}
	
	TextButton {
		id: back_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Back")
		width: 48 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			menu_stack.pop()
		}
	}
}
