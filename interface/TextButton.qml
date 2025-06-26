import QtQuick

ItemButton {
	id: button
	width: 96 * scale_factor
	height: 24 * scale_factor
	leftPadding: button.down ? horizontalPadding + 2 * scale_factor : horizontalPadding
	topPadding: button.down ? verticalPadding + 2 * scale_factor : verticalPadding
	
	readonly property int text_content_width: Math.floor(implicitContentWidth)
}
