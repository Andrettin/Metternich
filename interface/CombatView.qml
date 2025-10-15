import QtQuick
import QtQuick.Controls

Item {
	id: combat_view
	
	property string status_text: ""
	property string middle_status_text: ""
	property string right_status_text: ""
	readonly property int tile_size: metternich.defines.tile_size.width * scale_factor
	
	Rectangle {
		id: combat_map_background
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: left_bar.right
		anchors.right: right_bar.left
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
	
	RightBar {
		id: right_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: left_bar.right
		anchors.right: right_bar.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: left_bar.right
		anchors.right: right_bar.left
	}
	
	Connections {
		target: metternich.game
		
		function onCombat_running_changed() {
			if (!metternich.game.combat_running) {
				menu_stack.pop()
			}
		}
	}
}
