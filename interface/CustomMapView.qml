import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning
import map_country_model 1.0
import map_grid_model 1.0
import map_province_model 1.0
import map_site_model 1.0
import "./dialogs"

Item {
	id: map_view
	
	property string status_text: ""
	property string middle_status_text: ""
	property string interface_style: "dwarven"
	readonly property int tile_size: metternich.defines.tile_size.width * scale_factor
	readonly property real map_area_start_x: map.contentX / tile_size
	readonly property real map_area_start_y: map.contentY / tile_size
	readonly property real map_area_tile_width: map.width / tile_size
	readonly property real map_area_tile_height: map.height / tile_size
	
	property var selected_civilian_unit: null
	property var selected_site: null
	property var selected_province: null
	property bool selected_garrison: false
	
	property bool tile_detail_mode: false
	
	readonly property var event_dialog_component: Qt.createComponent("dialogs/EventDialog.qml")
	
	Rectangle {
		id: map_view_background
		color: "black"
		anchors.fill: parent
	}
	
	TableView {
		id: map
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: infopanel.right
		anchors.right: right_bar.left
		leftMargin: 0
		rightMargin: 0
		topMargin: 0
		bottomMargin: 0
		contentWidth: tile_size * metternich.map.width
		contentHeight: tile_size * metternich.map.height
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		visible: true
		model: MapGridModel {}
		delegate: TileView {}
		
		function pixel_to_tile_pos(pixel_x, pixel_y) {
			var tile_x = Math.floor(pixel_x / tile_size)
			var tile_y = Math.floor(pixel_y / tile_size)
			
			return Qt.point(tile_x, tile_y)
		}
		
		function center_on_tile(tile_x, tile_y) {
			var pixel_x = tile_x * tile_size - map.width / 2
			var pixel_y = tile_y * tile_size - map.height / 2
			
			map.contentX = Math.min(Math.max(pixel_x, 0), map.contentWidth - map.width)
			map.contentY = Math.min(Math.max(pixel_y, 0), map.contentHeight - map.height)
		}
		
		function center_on_country_capital(country) {
			var capital = country.game_data.capital
			
			if (capital === null) {
				return
			}
			
			var capital_game_data = capital.game_data
			var capital_x = capital_game_data.tile_pos.x
			var capital_y = capital_game_data.tile_pos.y
			
			center_on_tile(capital_x, capital_y)
		}
	}
	
	RightBar {
		id: right_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: infopanel.right
		anchors.right: right_bar.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: menu_button_bar.right
		anchors.right: right_bar.left
	}
	
	MenuButtonBar {
		id: menu_button_bar
		anchors.top: parent.top
		anchors.left: parent.left
	}
	
	MinimapArea {
		id: minimap_area
		anchors.top: menu_button_bar.bottom
		anchors.left: parent.left
	}
	
	InfoPanel {
		id: infopanel
		anchors.top: minimap_area.bottom
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	MenuDialog {
		id: menu_dialog
	}
	
	ResearchChoiceDialog {
		id: research_choice_dialog
	}
	
	AdvisorChoiceDialog {
		id: advisor_choice_dialog
	}
	
	LeaderChoiceDialog {
		id: leader_choice_dialog
	}
	
	CharacterDialog {
		id: character_dialog
	}
	
	ModifierDialog {
		id: modifier_dialog
	}
	
	GarrisonDialog {
		id: garrison_dialog
	}
	
	Keys.onPressed: function(event) {
		switch (event.key) {
			case Qt.Key_E:
				infopanel.end_turn_button.down = true
				break
			case Qt.Key_Shift:
				tile_detail_mode = true
				break
		}
	}
	
	Keys.onReleased: function(event) {
		if (event.isAutoRepeat) {
			return
		}
		
		switch (event.key) {
			case Qt.Key_E:
				infopanel.end_turn_button.down = undefined
				infopanel.end_turn_button.clicked()
				break
			case Qt.Key_Shift:
				tile_detail_mode = false
				break
		}
	}
	
	Connections {
		target: metternich
		
		function onNotification_added(title, portrait_object, text) {
			add_notification(title, portrait_object, text, map_view)
		}
		
		function onEvent_fired(event_instance) {
			if (event_dialog_component.status == Component.Error) {
				console.error(event_dialog_component.errorString())
				return
			}
			
			var event_dialog = event_dialog_component.createObject(map_view, {
				event_instance: event_instance,
				interface_style: map_view.interface_style
			})
			
			event_dialog.open()
		}
		
		function onFree_technology_choosable(potential_technologies) {
			research_choice_dialog.potential_technologies = potential_technologies
			research_choice_dialog.free_technology = true
			research_choice_dialog.open()
		}
		
		function onNext_advisor_choosable(potential_advisors) {
			advisor_choice_dialog.potential_advisors = potential_advisors
			advisor_choice_dialog.open()
		}
		
		function onNext_leader_choosable(potential_leaders) {
			leader_choice_dialog.potential_leaders = potential_leaders
			leader_choice_dialog.open()
		}
	}
	
	Connections {
		target: metternich.game.player_country.game_data
		
		function onTechnology_researched(technology) {
			if (notification_dialog_component.status == Component.Error) {
				console.error(notification_dialog_component.errorString())
				return
			}
			
			var dialog = notification_dialog_component.createObject(map_view, {
				title: technology.discovery ? "Discovery" : "Technology Researched",
				portrait_object: metternich.game.player_country.game_data.interior_minister_portrait,
				text: technology.discovery ? ("Your Excellency, we have discovered " + technology.name + "!") : ("Your Excellency, our scholars have made a breakthrough in the research of the " + technology.name + " technology!"),
				second_button_text: "View Technologies",
				second_button_effects: () => {
					if (!technology.discovery && !(menu_stack.currentItem instanceof TechnologyView)) {
						menu_stack.push("TechnologyView.qml")
					}
				}
			})
			
			dialog.open()
		}
	
		function onTechnology_lost(technology) {
			if (notification_dialog_component.status == Component.Error) {
				console.error(notification_dialog_component.errorString())
				return
			}
			
			var dialog = notification_dialog_component.createObject(map_view, {
				title: "Technology Lost",
				portrait_object: metternich.game.player_country.game_data.interior_minister_portrait,
				text: "Your Excellency, we have lost knowledge of the " + technology.name + " technology!",
				second_button_text: "View Technologies",
				second_button_effects: () => {
					if (!technology.discovery && !(menu_stack.currentItem instanceof TechnologyView)) {
						menu_stack.push("TechnologyView.qml")
					}
				}
			})
			
			dialog.open()
		}
		
		function onAdvisor_recruited(advisor) {
			if (notification_dialog_component.status == Component.Error) {
				console.error(notification_dialog_component.errorString())
				return
			}
			
			var dialog = notification_dialog_component.createObject(map_view, {
				title: "Advisor Recruited",
				portrait_object: metternich.game.player_country.game_data.interior_minister_portrait,
				text: "Your Excellency, " + advisor.full_name  + " has joined our nation as an advisor!",
				second_button_text: "View Advisors",
				second_button_effects: () => {
					politics_view_mode = PoliticsView.Mode.Advisors
					
					menu_stack.push("PoliticsView.qml", {
						country: metternich.game.player_country
					})
				}
			})
			
			dialog.open()
		}
		
		function onLeader_recruited(leader) {
			if (notification_dialog_component.status == Component.Error) {
				console.error(notification_dialog_component.errorString())
				return
			}
			
			var dialog = notification_dialog_component.createObject(map_view, {
				title: leader.leader_type_name + " Recruited",
				portrait_object: metternich.game.player_country.game_data.war_minister_portrait,
				text: "Your Excellency, the " + leader.leader_type_name.toLowerCase() + " " + leader.full_name  + " has joined our nation!"
			})
			
			dialog.open()
		}
		
		function onJournal_entry_completed(journal_entry) {
			if (notification_dialog_component.status == Component.Error) {
				console.error(notification_dialog_component.errorString())
				return
			}
			
			var dialog = notification_dialog_component.createObject(map_view, {
				title: journal_entry.name,
				portrait_object: journal_entry.portrait,
				text: journal_entry.description,
				effects_text: journal_entry.get_completion_effects_string(metternich.game.player_country)
			})
			
			dialog.open()
		}
	}
	
	onSelected_garrisonChanged: {
		metternich.clear_selected_military_units()
	}
	
	Component.onCompleted: {
		map.center_on_country_capital(metternich.game.player_country)
	}
}
