#pragma once

#include <QGeoCoordinate>
#include <QGeoPolygon>
#include <QGeoRectangle>

namespace metternich::geocoordinate {

inline double longitude_to_pixel_longitude(const double longitude, const double lon_per_pixel)
{
	//convert longitude to the longitude of the center point of its pixel
	return std::round(longitude / lon_per_pixel) * lon_per_pixel;
}

inline double latitude_to_pixel_latitude(const double latitude, const double lat_per_pixel)
{
	//convert latitude to the latitude of the center point of its pixel
	return std::round(latitude / lat_per_pixel) * lat_per_pixel;
}

template <typename number_type = int>
inline number_type longitude_to_x(const double longitude, const double lon_per_pixel)
{
	const double x = (longitude + 180.0) / lon_per_pixel;

	if constexpr (std::is_integral_v<number_type>) {
		return static_cast<number_type>(std::round(x));
	} else {
		return x;
	}
}

template <typename number_type = int>
inline number_type latitude_to_y(const double latitude, const double lat_per_pixel)
{
	const double y = (latitude * -1 + 90.0) / lat_per_pixel;

	if constexpr (std::is_integral_v<number_type>) {
		return static_cast<number_type>(std::round(y));
	} else {
		return y;
	}
}

inline QPoint to_point(const QGeoCoordinate &coordinate, const double lon_per_pixel, const double lat_per_pixel)
{
	using underlying_type = int;
	const underlying_type x = geocoordinate::longitude_to_x<underlying_type>(coordinate.longitude(), lon_per_pixel);
	const underlying_type y = geocoordinate::latitude_to_y<underlying_type>(coordinate.latitude(), lat_per_pixel);
	return QPoint(x, y);
}

inline QPointF to_pointf(const QGeoCoordinate &coordinate, const double lon_per_pixel, const double lat_per_pixel)
{
	using underlying_type = double;
	const underlying_type x = geocoordinate::longitude_to_x<underlying_type>(coordinate.longitude(), lon_per_pixel);
	const underlying_type y = geocoordinate::latitude_to_y<underlying_type>(coordinate.latitude(), lat_per_pixel);
	return QPointF(x, y);
}

inline QPointF to_circle_point(const QGeoCoordinate &coordinate)
{
	return QPointF(coordinate.longitude(), coordinate.latitude() * 2 * -1);
}

extern QPointF to_circle_edge_point(const QGeoCoordinate &coordinate);

/**
**	@brief	Get whether a coordinate is in a georectangle (presuming the rectangle is valid)
*/
inline bool is_in_georectangle(const QGeoCoordinate &coordinate, const QGeoRectangle &georectangle)
{
	const double lat = coordinate.latitude();
	const double lon = coordinate.longitude();
	const QGeoCoordinate bottom_left = georectangle.bottomLeft();
	const QGeoCoordinate top_right = georectangle.topRight();
	return lat >= bottom_left.latitude() && lat <= top_right.latitude() && lon >= bottom_left.longitude() && lon <= top_right.longitude();
}

template <typename T>
inline QString path_to_svg_string(const T &geocoordinate_path, const double lon_per_pixel, const double lat_per_pixel, const QRectF &bounding_rect)
{
	static_assert(std::is_same_v<T::value_type, QGeoCoordinate>);

	QString svg;

	for (int i = 0; i < geocoordinate_path.size(); ++i) {
		const QGeoCoordinate &geocoordinate = geocoordinate_path[i];

		if (i == 0) {
			svg += "M ";
		} else {
			svg += "L ";
		}

		const QPointF pos = geocoordinate::to_pointf(geocoordinate, lon_per_pixel, lat_per_pixel) - bounding_rect.topLeft();
		svg += QString::number(pos.x()) + " ";
		svg += QString::number(pos.y()) + " ";
	}

	return svg;
}

}
