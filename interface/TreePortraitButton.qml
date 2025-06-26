import QtQuick
import QtQuick.Controls

BaseTreePortraitButton {
	readonly property var entry: model.modelData
	readonly property var entry_parent: entry.tree_parent
	
	button_x: entry.tree_x
	button_y: entry.tree_y
	button_width: entry.tree_width
	has_tree_parent: entry !== null && entry.tree_parent !== null
	parent_button_x: entry_parent ? entry_parent.tree_x : 0
	parent_button_y: entry_parent ? entry_parent.tree_y : 0
	parent_button_width: entry_parent ? entry_parent.tree_width : 1
	parent_name: entry_parent ? entry_parent.name : ""
}
