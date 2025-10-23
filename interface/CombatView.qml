import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: combat_view
	
	property string status_text: ""
	property string middle_status_text: ""
	property string right_status_text: ""
	readonly property int tile_size: metternich.defines.tile_size.width * scale_factor
	readonly property var combat: metternich.game.current_combat
	readonly property var event_dialog_component: Qt.createComponent("dialogs/EventDialog.qml")
	property int popup_count: 0
	
	Rectangle {
		id: combat_map_background
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: left_bar.right
		anchors.right: infopanel.left
		color: Qt.rgba(0.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0, 1)
	}
	
	CombatTileMap {
		id: combat_map
		anchors.top: combat_map_background.top
		anchors.bottom: combat_map_background.bottom
		anchors.left: combat_map_background.left
		anchors.right: combat_map_background.right
	}
	
	LeftBar {
		id: left_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	CombatInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: left_bar.right
		anchors.right: infopanel.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: left_bar.right
		anchors.right: infopanel.left
	}
	
	Connections {
		target: metternich
		
		function onCombat_notification_added(title, portrait_object, text) {
			add_notification(title, portrait_object, text, combat_view)
			popup_count += 1
		}
		
		function onEvent_fired(event_instance) {
			if (event_dialog_component.status == Component.Error) {
				console.error(event_dialog_component.errorString())
				return
			}
			
			if (!event_instance.in_combat) {
				return
			}
			
			var event_dialog = event_dialog_component.createObject(combat_view, {
				event_instance: event_instance
			})
			
			event_dialog.open()
			popup_count += 1
		}
	}
	
	Connections {
		target: metternich.game
		
		function onCombat_running_changed() {
			if (!metternich.game.combat_running && popup_count === 0) {
				menu_stack.pop()
			}
		}
	}
	
	Component.onCompleted: {
		metternich.game.current_combat.start()
	}
	
	function on_popup_closed() {
		popup_count -= 1
		
		if (!metternich.game.combat_running && popup_count === 0) {
			menu_stack.pop()
		}
	}
}
