import QtQuick
import QtQuick.Controls

Row {
	id: feature_row
	height: 32 * scale_factor
	spacing: 4 * scale_factor
	
	property var site: null
	
	Repeater {
		model: site ? site.game_data.features : []
		
		FeatureImage {
			feature: model.modelData
			site: feature_row.site
		}
	}
}
