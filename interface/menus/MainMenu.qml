import QtQuick
import QtQuick.Controls
import ".."

MenuBase {
	id: main_menu
	title: qsTr("Main Menu")
	
	TextButton {
		id: play_random_map_button
		anchors.horizontalCenter: play_scenario_button.horizontalCenter
		anchors.bottom: play_scenario_button.top
		anchors.bottomMargin: 8 * scale_factor
		text: qsTr("Play Random Map")
		width: 128 * scale_factor
		
		onClicked: {
			menu_stack.push("RandomMapMenu.qml")
		}
	}
	
	TextButton {
		id: play_scenario_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		text: qsTr("Play Scenario")
		width: 128 * scale_factor
		
		onClicked: {
			menu_stack.push("ScenarioMenu.qml")
		}
	}
	
	TextButton {
		id: exit_button
		anchors.horizontalCenter: play_scenario_button.horizontalCenter
		anchors.top: play_scenario_button.bottom
		anchors.topMargin: 8 * scale_factor
		text: qsTr("Exit")
		width: 128 * scale_factor
		
		onClicked: {
			window.close()
		}
	}
}
