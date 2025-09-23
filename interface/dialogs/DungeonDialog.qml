import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

DialogBase {
	id: dungeon_dialog
	height: button_column.y + button_column.height + 8 * scale_factor
	title: dungeon ? dungeon.name : ""
	
	property var site: null
	property var dungeon: site ? site.game_data.dungeon : null
	
	PortraitButton {
		id: dungeon_portrait
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: dungeon ? dungeon.portrait.identifier : ""
	}
	
	SmallText {
		id: text_label
		anchors.top: dungeon_portrait.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: dungeon ? format_text("Do you wish to explore " + dungeon.name + "?"
			+ (dungeon.level !== 0 ? ("\n\nDungeon Level: " + dungeon.level) : "")
		) : ""
		wrapMode: Text.WordWrap
		width: Math.min(text_label_proxy.contentWidth, parent.width - 16 * scale_factor)
		
		SmallText { //used to measure text, avoiding the binding loop of using the main text label's content width directly, given the wrap mode
			id: text_label_proxy
			anchors.horizontalCenter: parent.horizontalCenter
			text: text_label.text
			opacity: 0
		}
	}
	
	Column {
		id: button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: yes_button
			text: "Yes"
			onClicked: {
				metternich.game.player_country.game_data.visit_target_site = site
				dungeon_dialog.close()
			}
		}
		
		TextButton {
			id: no_button
			text: "No"
			onClicked: {
				if (metternich.game.player_country.game_data.visit_target_site === site) {
					metternich.game.player_country.game_data.visit_target_site = null
				}
				dungeon_dialog.close()
			}
		}
	}
	
	onClosed: {
		site = null
	}
}
