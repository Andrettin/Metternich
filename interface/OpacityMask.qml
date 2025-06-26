import QtQuick
import QtQuick.Controls

Rectangle {
	id: opacity_mask
	color: "#ff0000"
	border.width: 0
	layer.enabled: true
	layer.samplerName: "maskSource"
	layer.effect: ShaderEffect {
		property variant source: opacity_mask.source
		fragmentShader: "../shaders/opacitymask.frag.qsb"
	}
	
	property var source: null
}
