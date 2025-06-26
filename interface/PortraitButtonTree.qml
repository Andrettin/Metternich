import QtQuick
import QtQuick.Controls

Flickable {
	id: portrait_button_tree
	contentWidth: contentItem.childrenRect.width + 8 * scale_factor //the extra padding is to provide some space for the scrollbar
	contentHeight: contentItem.childrenRect.height + 8 * scale_factor + 8 * scale_factor //the extra padding is to ensure there is enough space to display the text of the bottom-most button, plus some space for the scrollbar
	boundsBehavior: Flickable.StopAtBounds
	leftMargin: contentWidth < width ? ((width - contentWidth) / 2) : 0 //centralize the content horizontally if its width is less than that of the flickable
	clip: true
	
	property var entries: []
	property Component delegate
	
	Repeater {
		model: portrait_button_tree.entries
		delegate: portrait_button_tree.delegate
	}
}
