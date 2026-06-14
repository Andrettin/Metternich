import QtQuick
import QtQuick.Controls

Rectangle {
	id: scaled_image
	color: "#ff0000"
	border.width: 0
	layer.enabled: true
	layer.samplerName: "maskSource"
	layer.effect: ShaderEffect {
		property variant source: scaled_image.source
		property size outputSize: Qt.size(scaled_image.width, scaled_image.height)
		property size inputSize: Qt.size(scaled_image.source.width, scaled_image.source.height)
		fragmentShader: "../shaders/xbrz.frag.qsb"
	}
	
	property var source: null
}
