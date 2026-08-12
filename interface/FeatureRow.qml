import QtQuick
import QtQuick.Controls

Row {
	id: feature_row
	height: 32 * scale_factor
	spacing: 4 * scale_factor
	
	property var location: null
	
	Repeater {
		model: location ? location.game_data.features : []
		
		FeatureImage {
			feature: model.modelData
			location: feature_row.location
		}
	}
}
