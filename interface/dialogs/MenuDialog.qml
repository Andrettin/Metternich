import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import ".."

DialogBase {
	id: dialog
	title: "Menu"
	width: 256 * scale_factor
	height: content_height
	
	readonly property int content_height: button_column.y + button_column.height + 8 * scale_factor
	
	Column {
		id: button_column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: start_new_game_button
			text: "Start New Game"
			width: 128 * scale_factor
			onClicked: {
				dialog.close()
				menu_stack.pop()
				metternich.game.stop()
			}
		}
		
		TextButton {
			id: exit_game_button
			text: "Exit Game"
			width: 128 * scale_factor
			onClicked: {
				window.close()
			}
		}
		
		TextButton {
			id: cancel_button
			text: "Cancel"
			width: 128 * scale_factor
			onClicked: {
				dialog.close()
			}
		}
	}
}
