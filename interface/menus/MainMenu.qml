import QtQuick
import QtQuick.Controls
import ".."

MenuBase {
	id: main_menu
	title: qsTr("Main Menu")
	
	Column {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: play_scenario_button
			text: qsTr("Play Scenario")
			width: 128 * scale_factor
			
			onClicked: {
				menu_stack.push("ScenarioMenu.qml")
			}
		}
		
		TextButton {
			id: exit_button
			text: qsTr("Exit")
			width: 128 * scale_factor
			
			onClicked: {
				window.close()
			}
		}
	}
}
