#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

const double cEps = 1e-9;

class Line;

struct Point {
  double x;
  double y;

  Point(double x, double y) : x(x), y(y) {}
  Point(const Point& other) = default;
  ~Point() = default;

  double dist(const Point& other) const {
    return std::hypot(x - other.x, y - other.y);
  }

  double dist(const Line& line) const;

  double dist_squar(const Point& other) const {
    return (x - other.x) * (x - other.x) + (y - other.y) * (y - other.y);
  }

  static double crossProduct(const Point& x0, const Point& y0, const Point& x1) {
    return (y0.x - x0.x) * (x1.y - x0.y) - (y0.y - x0.y) * (x1.x - x0.x);
  }

  bool operator==(const Point& other) const {
    return dist(other) < cEps;
  }

  bool operator!=(const Point& other) const {
    return !((*this) == other);
  }

  void scale(const Point& center, double coef) {
    x = center.x + (x - center.x) * coef;
    y = center.y + (y - center.y) * coef;
  }

  void reflect(const Point& center) {
    scale(center, -1);
  }

  void reflect(const Line& line);

  void rotate(const Point& center, double angle) {
    double angle_rad = (M_PI * (angle / 180.0));
    double vec_x = (x - center.x);
    double vec_y = (y - center.y);
    x = center.x + vec_x * std::cos(angle_rad) - vec_y * std::sin(angle_rad); 
    y = center.y + vec_y * std::cos(angle_rad) + vec_x * std::sin(angle_rad);
  }
};


class Line {
  void Normalize() {
    double norm = std::hypot(a, b);
    if (norm > cEps) {
      a /= norm;
      b /= norm;
      c /= norm;
    }
  }

 public:
  double a;
  double b;
  double c;

  Line() : a(0.0), b(0.0), c(0.0) {}

  Line(const double k, const double l) : a(k), b(-1), c(l) {
    Normalize();
  }

  Line(const Point& p1, const Point& p2) {
    a = p2.y - p1.y;
    b = p1.x - p2.x;
    c = -(a * p1.x + b * p1.y);
    Normalize();
  }

  Line(const Point& p, const double k) : a(k), b(-1) {
    c = -(a * p.x + b * p.y);
    Normalize();
  }

  bool operator==(const Line& other) {
    double rat_a = a / other.a;
    double rat_b = b / other.b;
    double rat_c = c / other.c;
    return (std::fabs(rat_a - rat_b) < cEps && std::fabs(rat_b - rat_c) < cEps);
  }

  bool operator!=(const Line& other) {
    return !((*this) == other);
  }
};

double Point::dist(const Line& line) const {
  return std::fabs(line.a * x + line.b * y + line.c);
}

void Point::reflect(const Line& line) {
  double distance = line.a * x + line.b * y + line.c;
  x = x - 2 * line.a * distance;
  y = y - 2 * line.b * distance;
}

class Shape {
 public:
  virtual ~Shape() = default;

  virtual double perimeter() const = 0;

  virtual double area() const = 0;

  virtual bool operator==(const Shape& another) const = 0;
  virtual bool operator!=(const Shape& another) const = 0;

  virtual bool isCongruentTo(const Shape& another) const = 0;

  virtual bool isSimilarTo(const Shape& another) const = 0;

  virtual bool containsPoint(const Point& point) const = 0;

  virtual void rotate(const Point& center, double angle) = 0;

  virtual void reflect(const Point& center) = 0;

  virtual void reflect(const Line& axis) = 0;

  virtual void scale(const Point& center, double coefficient) = 0;
};

class Polygon : public Shape {
 protected:
  std::vector<Point> points;

  std::vector<double> GetCosAngles() const {
    size_t n = points.size();
    std::vector<double> cos_angles;
    for (size_t i = 0; i < n; ++i) {
      double ab = points[i].dist(points[(i + 1) % n]);
      double bc = points[(i + 1) % n].dist(points[(i + 2) % n]);
      double ac = points[(i + 2) % n].dist(points[i]);
      double cosAngle = (ab * ab + bc * bc - ac * ac) / (2 * ab * bc);
      cos_angles.push_back(cosAngle);
    }
    return cos_angles;
  }

  std::vector<double> GetSides() const {
    std::vector<double> sides;
    size_t n = points.size();
    for (size_t i = 0; i < n; ++i) {
      sides.push_back(points[i].dist(points[(i + 1) % n]));
    }
    return sides;
  }

  bool MatchParamsByCycle(const std::vector<double>& first,
                          const std::vector<double>& second,
                          double coef, size_t shift) const {
    bool match_right = true;
    bool match_left = true;
    size_t n = first.size();
    for (size_t i = 0; i < n; ++i) {
      if (std::fabs(first[i] - coef * second[(i + shift) % n]) > cEps) {
        match_right = false;
      }
      if (std::fabs(first[i] - coef * second[(n + shift - i) % n]) > cEps) {
        match_left = false;
      }
      if (!match_right && !match_left) {
        break;
      }
    }
    if (match_right || match_left) {
      return true;
    }
    return false;
  }

  bool IsCongruentParams(const std::vector<double>& first,
                          const std::vector<double>& second) const {
    size_t n = first.size();
    for (size_t shift = 0; shift < n; ++shift) {
      if (MatchParamsByCycle(first, second, 1, shift)) {
        return true;
      }
    }
    return false;
  }

  bool IsSimilarParams(const std::vector<double>& first,
                          const std::vector<double>& second) const {
    size_t n = first.size();
    for (size_t shift = 0; shift < n; ++shift) {
      double coef = first[0] / second[shift];
      if (MatchParamsByCycle(first, second, coef, shift)) {
        return true;
      }
    }
    return false;
  }

 public:
  explicit Polygon(const std::vector<Point>& points) : points(points) {}
  Polygon() = default;

  template <typename... Args>
  explicit Polygon(Args... args) : points({args...}) {}

  size_t verticesCount() { return points.size(); }

  std::vector<Point> getVertices() const { return points; }

  bool isConvex() const {
    size_t n = points.size();
    if (n < 3) return false;
    bool sign_cross_mult = false;
    for (size_t i = 0; i < n; ++i) {
      double cross_prod = Point::crossProduct(points[i], points[(i + 1) % n], points[(i + 2) % n]);
      if (i == 0) {
        sign_cross_mult = cross_prod > 0;
      } else {
        if (sign_cross_mult != (cross_prod > 0)) {
          return false;
        }
      }
    } 
    return true;
  }

  virtual double perimeter() const override {
    double per = 0;
    size_t n = points.size();
    for (size_t i = 0; i < n; ++i) {
      per += points[i].dist(points[(i + 1) % n]);
    }
    return per;
  }

  virtual double area() const override {
    double area = 0;
    size_t n = points.size();
    for (size_t i = 0; i < n; ++i) {
      area += (points[i].x * points[(i + 1) % n].y) -
              (points[i].y * points[(i + 1) % n].x);
    }
    return std::fabs(area / 2);
  }

  virtual bool operator==(const Shape& other) const override {
    const Polygon* other_pol = dynamic_cast<const Polygon*>(&other);
    if (!other_pol) {
      return false;
    }

    if (points.size() != other_pol->points.size()) {
      return false;
    }

    std::vector<Point> this_points_sorted = points;
    std::vector<Point> other_points_sorted = other_pol->points;

    auto cmp = [](const Point& p1, const Point& p2) -> bool {
      if (std::fabs(p1.x - p2.x) > cEps) {
        return p1.x < p2.x;
      }
      return p1.y < p2.y;
    };

    std::sort(this_points_sorted.begin(), this_points_sorted.end(), cmp);
    std::sort(other_points_sorted.begin(), other_points_sorted.end(), cmp);

    for (size_t i = 0; i < this_points_sorted.size(); ++i) {
      if (!(this_points_sorted[i] == other_points_sorted[i])) {
        return false;
      }
    }
    return true;
  }

  virtual bool operator!=(const Shape& other) const override {
    return !((*this) == other);
  }

  virtual bool isCongruentTo(const Shape& other) const override {
    const Polygon* other_pol = dynamic_cast<const Polygon *>(&other);
    if (!other_pol || points.size() != other_pol->points.size()) {
      return false;
    }
    if (!IsCongruentParams(GetSides(), other_pol->GetSides())) {
      return false;
    }
    if (!IsCongruentParams(GetCosAngles(), other_pol->GetCosAngles())) {
      return false;
    }
    return true;
  }

  virtual bool isSimilarTo(const Shape& other) const override {
    const Polygon* other_pol = dynamic_cast<const Polygon *>(&other);
    if (!other_pol || points.size() != other_pol->points.size()) {
      return false;
    }
    if (!IsSimilarParams(GetSides(), other_pol->GetSides())) {
      return false;
    }
    if (!IsSimilarParams(GetCosAngles(), other_pol->GetCosAngles())) {
      return false;
    }
    return true;
  }

  bool containsPoint(const Point& p) const override {
    size_t n = points.size();
    if (n < 3) return false;
    bool sign_cross_mult = false;
    for (size_t i = 0; i < n; ++i) {
      double cross_prod = Point::crossProduct(points[i], points[(i + 1) % n], p);
      if (i == 0) {
        sign_cross_mult = cross_prod >= 0;
      } else {
        if (sign_cross_mult != (cross_prod >= 0)) {
          return false;
        }
      }
    } 
    return true;
  }

  virtual void rotate(const Point& center, double angle) override{
    for (size_t i = 0; i < points.size(); ++i) {
      points[i].rotate(center, angle);
    }
  }

  virtual void reflect(const Point& center) override {
    for (size_t i = 0; i < points.size(); ++i) {
      points[i].reflect(center);
    }
  }

  virtual void reflect(const Line& line) override {
    for (size_t i = 0; i < points.size(); ++i) {
      points[i].reflect(line);
    }
  }

  virtual void scale(const Point& center, double coef) override {
    for (size_t i = 0; i < points.size(); ++i) {
      points[i].scale(center, coef);
    }
  }
};

class Ellipse : public Shape {
  Point f1;
  Point f2;
  double a;
  double b;

 public:
  Ellipse(const Point& f1, const Point& f2, double sum) : f1(f1), f2(f2) {
    b = sum / 2;
    double d = f1.dist(center());
    a = std::sqrt(b * b - d * d);
  }

  Point center() const { return Point((f1.x + f2.x) / 2, (f1.y + f2.y) / 2); }

  std::pair<Point,Point> focuses() const { return std::make_pair(f1, f2); }

  double eccentricity() const { return f1.dist(center()) / b; }

  static std::pair<Line, Line> directrices() { return {Line(), Line()}; }

  virtual double area() const override {
    return M_PI * a * b;
  }

  virtual double perimeter() const override {
    return M_PI * (3 * (a + b) - std::sqrt((3 * a + b) * (a + 3 * b)));
  }

  virtual bool operator==(const Shape& other) const override {
    const auto* other_el = dynamic_cast<const Ellipse*>(&other);
    if (!other_el) {  
      return false;
    }
    if ((a == other_el->a) && (b == other_el->b) &&
        (((f1 == other_el->f1) && (f2 == other_el->f2)) ||
        (((f1 == other_el->f2) && (f2 == other_el->f1))))) {
      return true;
    }
    return false;
  }

  virtual bool operator!=(const Shape& other) const override {
    return !((*this) == other);
  }

  virtual bool isCongruentTo(const Shape& other) const override {
    const auto* other_el = dynamic_cast<const Ellipse*>(&other);
    if (!other_el) {
      return false;
    }
    return (std::fabs(a - other_el->a) < cEps) && (std::fabs(b - other_el->b));
  }

  virtual bool isSimilarTo(const Shape& other) const override {
    const auto* other_el = dynamic_cast<const Ellipse*>(&other);
    if (!other_el) {
      return false;
    }
    return std::fabs((a / other_el->a) - (b / other_el->b)) < cEps;
  }

  virtual bool containsPoint(const Point& point) const override {
    return std::fabs(point.dist(f1) + point.dist(f2) - a - b) < cEps;
  }

  virtual void rotate(const Point& center, double angle) override {
    f1.rotate(center, angle);
    f2.rotate(center, angle);
  }

  virtual void reflect(const Point& center) override {
    f1.reflect(center);
    f2.reflect(center);
  }

  virtual void reflect(const Line& line) override {
    f1.reflect(line);
    f2.reflect(line);
  }

  virtual void scale(const Point& center, double coef) override{
    a *= std::fabs(coef);
    b *= std::fabs(coef);
    f1.scale(center, coef);
    f2.scale(center, coef);
  }
};

class Circle : public Shape {
  Point center_circ;
  double r;

 public:
  Circle(const Point& center, double r) : center_circ(center), r(r) {
    std::cerr << center.x << " " << center.y << " " << r << std::endl;
  }

  double radius() const { return r; }

  Point center() const { return center_circ; }

  virtual double perimeter() const override { return 2 * M_PI * r; }

  virtual double area() const override { return M_PI * r * r; }

  virtual bool operator==(const Shape& other) const override {
    const Circle* other_circle = dynamic_cast<const Circle*>(&other);
    if (!other_circle) {
      return false;
    }
    return (center_circ == other_circle->center_circ) && (std::fabs(r - other_circle->r) < cEps);
  }

  virtual bool operator!=(const Shape& other) const override {
    return !((*this) == other);
  }

  virtual bool isCongruentTo(const Shape& other) const override {
    const Circle* other_circle = dynamic_cast<const Circle*>(&other);
    if (!other_circle) {
      return false;
    }
    return std::fabs(r - other_circle->r) < cEps;
  }

  virtual bool isSimilarTo(const Shape& other) const override {
    const Circle* other_circle = dynamic_cast<const Circle*>(&other);
    if (!other_circle) {
      return false;
    }
    return true;
  }

  virtual bool containsPoint(const Point& point) const override { return (point.dist(center_circ) - r) < cEps;}

  virtual void rotate(const Point& center, double angle) override { center_circ.rotate(center, angle); }

  virtual void reflect(const Point& center) override { center_circ.reflect(center); }

  virtual void reflect(const Line& line) override { center_circ.reflect(line); }

  virtual void scale(const Point& center, double coef) override {
    center_circ.scale(center, coef);
    r *= std::fabs(coef);
  }
};

class Rectangle : public Polygon {
 public:
  Rectangle(const Point& p1, const Point& p2, double coef) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double length = p1.dist(p2);
    Point pb = Point(0, 0);
    Point pa = Point(0, 0);
    coef = coef < 1 ? coef : 1.0 / coef;
    double side2 = length / std::sqrt(1 + coef * coef);
    double side1 = side2 * coef;
    if (dx * dy < 0 or dy == 0) {
      Point min = p1.x < p2.x ? p1 : p2;
      double coord_angle = atan2(fabs(dy), fabs(dx));
      double rect_angle = atan2(side1, side2);
      double angle = fabs(coord_angle - rect_angle);
      pb = Point({min.x - cos(angle) * side2, min.y + sin(angle) * side2});
      pa = Point({min.x + cos(M_PI / 2 - angle) * side1,
                  min.y + sin(M_PI / 2 - angle) * side1});
    } else if (dx * dy > 0 or dx == 0) {
      Point min = p1.y < p2.y ? p1 : p2;
      double coord_angle = atan2(fabs(dy), fabs(dx));
      double rect_angle = atan2(side1, side2);
      double angle = fabs(coord_angle + rect_angle);
      pb = Point({min.x + cos(angle) * side2, min.y + sin(angle) * side2});
      pa = Point({min.x + cos(M_PI / 2 - angle) * side1,
                  min.y - sin(M_PI / 2 - angle) * side1});
    }
    points.push_back(p1);
    points.push_back(pb);
    points.push_back(p2);
    points.push_back(pa);
  }

  Point center() const {
    Point p1 = points[0];
    Point p2 = points[2];
    return Point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
  }

  std::pair<Line, Line> diagonals() const {
    if (points.size() != 4) return {Line(), Line()};
    return {Line(points[0], points[2]), Line(points[1], points[3])};
  }
};

class Square : public Rectangle {
 public:
  Square(const Point& p1, const Point& p2) : Rectangle(p1, p2, 1) {}

  Circle circumscribedCircle() const {
    Point center = Rectangle::center();
    return Circle(center, center.dist(points[0]));
  }

  Circle inscribedCircle() const {
    Point center = Rectangle::center();
    return Circle(center, center.dist(Line(points[0], points[1])));
  }
};

class Triangle : public Polygon {
 public:
  Triangle(const Point& p1, const Point& p2, const Point& p3) : Polygon(p1, p2, p3) {
    std::cerr << p1.x << " " << p1.y << "\n" << p2.x << " " << p2.y << "\n" << p3.x << " " << p3.y << std::endl;
  }

  Circle circumscribedCircle() const {
    double x0 = points[0].x;
    double y0 = points[0].y;
    double x1 = points[1].x;
    double y1 = points[1].y;
    double x2 = points[2].x;
    double y2 = points[2].y;

    double k = 2 * (x0 * (y1 - y2) + x1 * (y2 - y0) + x2 * (y0 - y1));
    if (std::fabs(k) < cEps) {
      return Circle(Point(0, 0), 0);
    }

    double x = ((x0 * x0 + y0 * y0) * (y1 - y2) + (x1 * x1 + y1 * y1) * (y2 - y0) +
                (x2 * x2 + y2 * y2) * (y0 - y1)) /
               k;
    double y = ((x0 * x0 + y0 * y0) * (x2 - x1) + (x1 * x1 + y1 * y1) * (x0 - x2) +
                (x2 * x2 + y2 * y2) * (x1 - x0)) /
               k;
    Point center(x, y);
    return Circle(center, center.dist(points[0]));
  }

  Circle inscribedCircle() const {
    Point p1 = points[0];
    Point p2 = points[1];
    Point p3 = points[2];
    double a = p2.dist(p3);
    double b = p3.dist(p1);
    double c = p1.dist(p2);
    double x_0 = (a * p1.x + b * p2.x + c * p3.x) / (a + b + c);
    double y_0 = (a * p1.y + b * p2.y + c * p3.y) / (a + b + c);
    Point center(x_0, y_0);
    return Circle(center, center.dist(Line(p1, p2)));
  }

  Point centroid() const {
    double x = 0;
    double y = 0;
    for (Point p : points) {
      x += p.x;
      y += p.y;
    }
    return Point(x / 3, y / 3);
  }

  double getSlope(const Point& p1, const Point& p2) const {
    if (std::fabs(p2.x - p1.x) < cEps) {
      return INFINITY;
    }
    return (p2.y - p1.y) / (p2.x - p1.x);
  }

  Point orthocenter() const {
    Point centroid = Triangle::centroid();
    Point circumcenter = circumscribedCircle().center();
    return Point(3 * centroid.x - 2 * circumcenter.x, 3 * centroid.y - 2 * circumcenter.y);
  }

  Line EulerLine() const { return Line(centroid(), orthocenter()); }

  Circle ninePointsCircle() const {
    Point p1((points[0].x + points[1].x) / 2, (points[0].y + points[1].y) / 2);
    Point p2((points[1].x + points[2].x) / 2, (points[1].y + points[2].y) / 2);
    Point p3((points[2].x + points[0].x) / 2, (points[2].y + points[0].y) / 2);
    return Triangle(p1, p2, p3).circumscribedCircle();
  }
};