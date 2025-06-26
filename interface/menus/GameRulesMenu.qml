import QtQuick
import QtQuick.Controls
import ".."

MenuBase {
	id: game_rules_menu
	title: qsTr("Game Rules")
	
	property var selected_scenario: null
	property bool rules_changed: false
	
	Grid {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: title_item.bottom
		anchors.topMargin: 32 * scale_factor
		anchors.bottom: previous_menu_button.top
		anchors.bottomMargin: 32 * scale_factor
		spacing: 16 * scale_factor
		
		Repeater {
			model: metternich.get_game_rule_groups()
			
			LabeledComboBox {
				id: game_rule_group_combobox
				text: qsTr(game_rule_group.name)
				currentIndex: get_game_rule_group_current_index(game_rule_group)
				tooltip: format_text(small_text(get_game_rule_group_description(game_rule_group)))
				visible: count > 2
				model: ["Disabled"].concat(object_list_to_name_list(game_rule_group.rules))
				onActivated: function(index) {
					for (var game_rule of game_rule_group.rules) {
						var rule_value = index > 0 && (game_rule_group.rules[index - 1] == game_rule)
						
						if (metternich.preferences.game_rules.get_value(game_rule) !== rule_value) {
							metternich.preferences.game_rules.set_value(game_rule, rule_value)
							rules_changed = true
						}
					}
				}
				
				readonly property var game_rule_group: modelData
			}
		}
		
		Repeater {
			model: metternich.preferences.game_rules.rules
			
			CustomCheckBox {
				id: game_rule_checkbox
				text: qsTr(game_rule.name)
				checked: metternich.preferences.game_rules.get_value(game_rule)
				checkable: true
				tooltip: small_text(game_rule.description)
				visible: metternich.preferences.game_rules.values.length > 0 && !game_rule.hidden && metternich.preferences.game_rules.is_rule_available(game_rule) && (game_rule.group === null || game_rule.group.rules.length === 1) //the check to the values property is there to cause a property binding reevaluation for the is_rule_available check
				onCheckedChanged: {
					if (metternich.preferences.game_rules.get_value(game_rule) !== checked) {
						metternich.preferences.game_rules.set_value(game_rule, checked)
						rules_changed = true
					}
				}
				
				readonly property var game_rule: model.modelData
			}
		}
	}
	
	TextButton {
		id: previous_menu_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Previous Menu")
		width: 96 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			if (rules_changed) {
				metternich.preferences.save()
				metternich.game.setup_scenario(selected_scenario).then(() => {
					menu_stack.pop()
				})
			} else {
				menu_stack.pop()
			}
		}
	}
	
	function get_game_rule_group_current_index(game_rule_group) {
		for (var i = 0; i < game_rule_group.rules.length; i++) {
			var game_rule = game_rule_group.rules[i]
			
			if (metternich.preferences.game_rules.get_value(game_rule)) {
				return i + 1
			}
		}
		
		return 0
	}
	
	function get_game_rule_group_description(game_rule_group) {
		var str = ""
		
		for (var game_rule of game_rule_group.rules) {
			if (str.length > 0) {
				str += "\n"
			}
			
			str += game_rule.name + "\n\t" + game_rule.description
		}
		
		return str
	}
}
