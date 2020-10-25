

#include <carbsup.h>

int _fltused = 0;


float _pow(float n, int ex) {

	float temp;
	if (ex == 0)
		return 1.f;

	temp = _pow(n, ex / 2);
	if ( ex % 2 == 0 ) {
		return temp * temp;
	}
	else {
		if ( ex > 0 ) {

			return n * temp * temp;
		}
		else {

			return ( temp * temp ) / n;
		}
	}
}