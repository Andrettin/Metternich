import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import ".."

Popup {
	id: dialog
	x: Math.floor(parent.width / 2 - (width / 2))
	y: Math.floor(parent.height / 2 - (height / 2))
	width: default_width
	height: default_height
	padding: 0
	modal: true
	dim: false
	focus: false
	closePolicy: Popup.NoAutoClose
	clip: true
	opacity: activeFocus ? 1 : 0
	
	readonly property int default_width: 256 * scale_factor
	readonly property int default_height: 256 * scale_factor
	property string interface_style: "default"
	property int panel: 1
	property string title: ""
	readonly property var title_item: title_text
	property bool open_when_menu_is_closed: false
	
	background: Item {
		Rectangle {
			anchors.fill: parent
			color: "black"
			radius: 5 * scale_factor
		}
		
		TiledBackground {
			id: tiled_background
			anchors.fill: parent
			layer.enabled: true
			visible: false
		}
	
		OpacityMask {
			anchors.fill: parent
			width: parent.width
			height: parent.height
			radius: 5 * scale_factor
			source: tiled_background
		}
		
		Rectangle {
			anchors.fill: parent
			color: "transparent"
			radius: 5 * scale_factor
			border.color: "gray"
			border.width: 1 * scale_factor
		}
	}
	
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true
		//prevent events from propagating below
	}
	
	Pane {
		id: pane
		anchors.fill: parent
		focusPolicy: Qt.ClickFocus
		background: null
	}
	
	LargeText {
		id: title_text
		text: dialog.title
		anchors.top: parent.top
		anchors.topMargin: 16 * scale_factor
		anchors.left: parent.left
		anchors.leftMargin: 8 * scale_factor
		anchors.right: parent.right
		anchors.rightMargin: 8 * scale_factor
		horizontalAlignment: Text.AlignHCenter
		wrapMode: Text.WordWrap
	}
	
	onOpened: {
		if (open_dialogs) {
			open_dialogs.push(this)
		}
		
		dialog.receive_focus()
	}
	
	onClosed: {
		dialog.give_up_focus()
		
		if (open_dialogs) {
			const dialog_index = open_dialogs.indexOf(this)
			if (dialog_index != -1) {
				open_dialogs.splice(dialog_index, 1)
			}
		}
	}
	
	function give_up_focus() {
		//give focus to a different open dialog, if any
		if (open_dialogs) {
			for (var i = open_dialogs.length - 1; i >= 0; --i) {
				var child_item = open_dialogs[i]
				
				if (child_item == this) {
					continue
				}
				
				if (child_item instanceof DialogBase) {
					child_item.receive_focus()
					return
				}
			}
		}
		
		parent.forceActiveFocus()
	}
	
	function receive_focus() {
		pane.forceActiveFocus()
		dialog.z = 1 + (open_dialogs ? open_dialogs.length : 0)
	}
	
	function calculate_max_button_width(button_container) {
		var max_button_width = 0
		var button_margin_width = 4 * scale_factor * 2
		
		for (var i = 0; i < button_container.children.length; ++i) {
			var child_item = button_container.children[i]
			var button_text_content_width = child_item.text_content_width
			
			if (button_text_content_width !== undefined) {
				max_button_width = Math.max(max_button_width, button_text_content_width + button_margin_width)
			}
		}
		
		return max_button_width
	}
}
