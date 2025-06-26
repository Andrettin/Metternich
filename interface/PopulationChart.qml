import QtQuick
import QtQuick.Controls
import QtCharts

ChartView {
	property var data_source: null //the data source for the chart, i.e. a population structure
	property var population_data: null //the data for the chart, e.g. population type counts; if present, overrides what is in the data source
	readonly property var pie_series: pie_series

	id: chart
	width: 64 * scale_factor
	height: 64 * scale_factor
	margins.top: 0
	margins.bottom: 0
	margins.left: 0
	margins.right: 0
	legend.visible: false
	backgroundColor: "transparent"
	antialiasing: true

	onData_sourceChanged: chart.update_chart()
	onPopulation_dataChanged: chart.update_chart()

	PieSeries {
		id: pie_series
		size: 0.95

		onHovered: function(slice, state) {
			if (state === true) {
				tooltip.text = small_text(slice.label + " (" + (slice.percentage * 100).toFixed(2) + "%)")
				tooltip.visible = true
			} else {
				tooltip.visible = false
			}
		}
	}
	
	CustomTooltip {
		id: tooltip
		visible: false
	}
}
