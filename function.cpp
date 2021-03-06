#include "function.h"

#ifndef INFINITY
#define INFINITY 1e200
#endif

QuarticFunction::QuarticFunction(const Network &n) : 
	net(n), to(n.getNVertex()) {
	FOR(i,n.arcs.size()) to[net.arcs[i].head].push_back(i);
}

Real QuarticFunction::f(Vector &v) const{
	Real sum = 0.0, x, c;
	int arc;
  
	assert(v.size()==net.arcs.size());

	FOR(i, v.size()){
		FOR(j, to[net.arcs[i].tail].size()){
			arc = to[net.arcs[i].tail][j];
			x = (v.coeff(arc)/net.arcs[arc].cap);
			sum += 20*x*x*x*x;
		}
		x = (v.coeff(i)/net.arcs[i].cap);
		sum += 100*x*x;
    
		c = Real(net.arcs[i].head+1.0)/Real(net.arcs[i].tail+1.0);
		sum += c*v.coeff(i);
	}
	return sum;
}

Vector QuarticFunction::g(Vector &v) const{
	Vector d(v.size());
	Real x, c, sum;
	int a;

	FOR(i, v.size()) {
		sum = 0;
		FOR(j, to[net.arcs[i].tail].size()){
			a = to[net.arcs[i].tail][j];
			x = v.coeff(a); c = net.arcs[a].cap;
			d.coeffRef(a) += 80*x*x*x/(c*c*c*c);
		}

		x = v.coeff(i); c = net.arcs[i].cap;
		d.coeffRef(i) += 200*x/(c*c);

		c = Real(net.arcs[i].head+1.0)/Real(net.arcs[i].tail+1.0);
		d.coeffRef(i) += c;
	}
	return d;
}

//!!!! have not debugged
// do not use yet
Vector QuarticFunction::gg(Vector &v) const{
	Vector d(v.size());
	Real x, c, sum;
	int a;

	FOR(i, v.size()) {
		sum = 0;
		FOR(j, to[net.arcs[i].tail].size()){
			a = to[net.arcs[i].tail][j];
			x = v.coeff(a); c = net.arcs[a].cap;
			d.coeffRef(a) += 240*x*x/(c*c*c*c);
		}

		x = v.coeff(i); c = net.arcs[i].cap;
		d.coeffRef(i) += 200/(c*c);

		c = Real(net.arcs[i].head+1.0)/Real(net.arcs[i].tail+1.0);
		d.coeffRef(i) += c;
	}
	return d;
}

BPRFunction::BPRFunction(const MultiCommoNetwork &n, Real a, Real b): 
	net(n), alpha(a), beta(b) {}

Real BPRFunction::f(Vector &x) const {
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Real sum = 0.0, ya, ca, ta;
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index() / K;
		ca = net.arcs[a].cap;
		ta = net.arcs[a].cost;
		ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		sum += ta*ya*(1 + alpha/(beta+1)*pow(ya/ca,beta));
	}
	return sum;
}

Function* BPRFunction::reduced_function() const {
	return new ReducedBPRFunction(net);
}

Vector BPRFunction::reduced_variable(Vector &x) const {
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size());
	Vector y(A);
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index()/K;
		Real ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		y.insert(a) = ya;
	}
	return y;
}

Vector BPRFunction::g(Vector &x) const {
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Vector d(K*A);
	Real ya, ca, dd, ta;
	Vector::iterator itx = x.get_iterator();

	while(!itx.end()){
		int a = itx.index() / K;
		ca = net.arcs[a].cap;
		ta = net.arcs[a].cost;
		ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		dd = ta + ta*alpha*pow(ya/ca,beta);
		FOR(k, K) d.insert(a*K+k) = dd;

		int na = itx.end()? A : itx.index()/K;
		for(int aa = a+1; aa < na; aa++) 
			FOR(k, K)
				d.insert(aa*K+k) = net.arcs[aa].cost;		                                                         
	}
	return d;
}

Vector BPRFunction::gg(Vector &x) const {
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Vector d(K*A);
	Real ya, ca, dd, ta;
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index() / K;
		ca = net.arcs[a].cap;
		ta = net.arcs[a].cost;
		ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		dd = ta*alpha*beta*pow(ya/ca,beta-1)/ca;
		if(fabs(dd)>1e-7) FOR(k, K) d.insert(a*K+k) = dd;
	}
	return d;
}

ReducedBPRFunction::ReducedBPRFunction(const MultiCommoNetwork &n, Real a, Real b): 
	net(n), alpha(a), beta(b) {}

Real ReducedBPRFunction::f(Vector &x) const {
	int A = net.arcs.size();
	assert(A == x.size()); // debug
	Real sum = 0.0, ya, ca, ta;
	ITER(x, itx) {
		int a = itx.index();
		ya = itx.value();
		ca = net.arcs[a].cap;
		ta = net.arcs[a].cost;
		sum += ta*ya*(1 + alpha/(beta+1)*pow(ya/ca,beta));
	}
	return sum;
}

Vector ReducedBPRFunction::g(Vector &x) const {
	int A = net.arcs.size(), a = 0, pa = 0;
	assert(A == x.size()); // debug
	Vector d(A);
	Real ya, ca, ta;
	ITER(x, itx){
		pa = a;
		a = itx.index();
		for(int aa = pa+1; aa<a; aa++) d.insert(aa) = net.arcs[aa].cost;
		ya = itx.value(); 
		ca = net.arcs[a].cap; 
		ta = net.arcs[a].cost;
		d.insert(a) = ta + ta*alpha*pow(ya/ca,beta);
	}
	for(int aa = a+1; aa<A; aa++) d.insert(aa) = net.arcs[aa].cost;
	return d;
}

Vector ReducedBPRFunction::gg(Vector &x) const {
	int A = net.arcs.size();
	assert(A == x.size()); // debug
	Vector d(A);
	Real ya, ca, ta;
	ITER(x, itx) {
		int a = itx.index();
		ya = itx.value(); ca = net.arcs[a].cap; ta = net.arcs[a].cost;
		assert(ca>0.0);
		d.insert(a) = ta*alpha*beta*pow(ya/ca,beta-1)/ca;
	}
	return d;
}

Real KleinrockFunction::f(Vector &x) const{
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Real sum = 0.0, ya, ca;
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index() / K;
		ca = net.arcs[a].cap;
		ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		if(ca>ya) sum += ya/(ca-ya);
		else return INFINITY;
	}
	return sum;
}

Vector KleinrockFunction::g(Vector &x) const{
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Vector d(K*A);
	Real ya, ca, dd;
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index() / K;
		ca = net.arcs[a].cap;
		ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		dd = ca - ya;
		if(dd > 0) dd = ca/(dd*dd);
		else dd = INFINITY;
		FOR(k,K) d.insert(a*K + k) = dd;
	}
	return d;
}

Function* KleinrockFunction::reduced_function() const {
	return new ReducedKleinrockFunction(net);
}

Vector KleinrockFunction::reduced_variable(Vector &x) const {
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size());
	Vector y (A);
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index()/K;
		Real ya = 0.0;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K == a);
		y.insert(a) = ya;
	}
	return y;
}

Vector KleinrockFunction::gg(Vector &x) const{
	int K = net.commoflows.size(), A = net.arcs.size();
	assert(K*A == x.size()); // debug
	Vector d(K*A);
	Real ya, ca, dd;
	Vector::iterator itx = x.get_iterator();
	while(!itx.end()){
		int a = itx.index()/K;
		ya = 0.0; ca = net.arcs[a].cap;
		do ya += itx.value(), ++itx; while(!itx.end() && itx.index()/K==a);
		dd = ca - ya;
		//assert(ca-ya>0);
		if(ca-ya>0) dd = 2*ca/(dd*dd*dd);
		else dd = INFINITY;
		FOR(k,K) d.insert(a*K + k) = dd;
	}
	return d;
}

KleinrockFunction::KleinrockFunction(const MultiCommoNetwork &n) : net(n){
}


Real ReducedKleinrockFunction::f(Vector &x) const {
	int A = net.arcs.size();
	assert(A == x.size()); // debug
	Real sum = 0.0, ya, ca;
	ITER(x, itx){
		int a = itx.index();
		ya = itx.value();
		ca = net.arcs[a].cap;
		//assert(ca-ya>0);
		if(ca>ya) sum += ya/(ca-ya); else return INFINITY;
	}
	return sum;
}

Vector ReducedKleinrockFunction::g(Vector &x) const{
	int A = net.arcs.size();
	assert(A == x.size()); // debug
	Vector d(A);
	Real ya, ca, dd;
	ITER(x, itx){
		int a = itx.index();
		ya = itx.value(); ca = net.arcs[a].cap;
		//assert(ca-ya>0);
		dd = ca - ya;
		if(dd>0) dd = ca/(dd*dd);
		else dd = INFINITY;
		d.insert(a) = dd;
	}
	return d;
}

Vector ReducedKleinrockFunction::gg(Vector &x) const{
	int A = net.arcs.size();
	assert(A == x.size()); // debug
	Vector d(A);
	Real ya, ca, dd;
	ITER(x, itx){
		int a = itx.index();
		ya = itx.value(); ca = net.arcs[a].cap;
		//assert(ca-ya>0);
		dd = ca - ya;
		if(dd>0) dd = 2*ca/(dd*dd*dd);
		else dd = INFINITY;
		d.insert(a) = dd;
	}
	return d;
}

ReducedKleinrockFunction::ReducedKleinrockFunction(const MultiCommoNetwork &n) : net(n){
}


#define PHI 0.6180339887498948482045868343656

Real golden_section_search ( Vector &A,
                             Vector &B,
                             Function *obj,
                             int iterations)
{
	Vector *x1 = new Vector(A), *x4 = new Vector(B);
	Vector *x2 = new Vector(B), *x3 = new Vector(B), *xtmp;
  
	(*x2) -= A; (*x2) *= (1-PHI); (*x2) += A;
	(*x3) += A; (*x3) -= (*x2);

	Real f1 = obj->f(A), f4 = obj->f(B);
	Real f2 = obj->f(*x2), f3 = obj->f(*x3), fm = f1;
	Real b1 = 0, b4 = 1, b2 = 1-PHI, b3 = PHI, bm = 0;

	if(fm > f2) fm = f2, bm = b2;
	if(fm > f3) fm = f3, bm = b3;
	if(fm > f4) fm = f4, bm = b4;

	FOR(i, iterations){
		if(f2>f3){
			xtmp = x1; x1 = x2; x2 = x3; x3 = xtmp;
			f1 = f2; f2 = f3;
			b1 = b2; b2 = b3;

			(*x3) = (*x1); (*x3) += (*x4); (*x3) -= (*x2);
			f3 = obj->f(*x3);
			b3 = b1 + b4 - b2;      

			if(fm > f3) fm = f3, bm = b3;
		}
		else{
			xtmp = x4; x4 = x3; x3 = x2; x2 = xtmp;
			f4 = f3; f3 = f2;
			b4 = b3; b3 = b2;

			(*x2) = (*x1); (*x2) += (*x4); (*x2) -= (*x3);
			f2 = obj->f(*x2);
			b2 = b1 + b4 - b3;

			if(fm > f2) fm = f2, bm = b2;
		}
	}

	delete x1; delete x2; delete x3; delete x4;
	return bm;
}

extern fstream iteration_report;

Real general_section_search ( Vector &A,
                              Vector &B,
                              Function *obj,
                              int iterations,
                              Real b2, Real b3)
{
	//iteration_report << "general section search" << endl;
	Vector *x1 = new Vector(A), *x4 = new Vector(B);
	Vector *x2 = new Vector(B), *x3 = new Vector(B), *xtmp;
  
	(*x2) -= A; (*x2) *= b2; (*x2) += A;
	(*x3) -= A; (*x3) *= b3; (*x3) += A;

	Real f1 = obj->f(A), f4 = obj->f(B);
	Real f2 = obj->f(*x2), f3 = obj->f(*x3), fm = f1;
	Real fbound=0, fnewbound;
	Real b1 = 0, b4 = 1, bm = 0, tmp;

	if(fm > f2) fm = f2, bm = b2;
	if(fm > f3) fm = f3, bm = b3;
	if(fm > f4) fm = f4, bm = b4;

	FOR(i, iterations){
		// make sure that the order is x1 -> x2 -> x3 -> x4
		if(b2>b3){
			xtmp = x2; x2 = x3; x3 = xtmp;
			tmp  = b2; b2 = b3; b3 = tmp;
			tmp  = f2; f2 = f3; f3 = tmp;
		}

		// safe guard the case where x2 and x3 are too close to each other
		if(fabs(b2-b3)<1e-12 && fabs(f2-f3) < 1e-6){
			iteration_report << "x2 and x3 too close at iteration "<<i<<endl;
			(*x3) -= (*x4); (*x3) *= (PHI-1); (*x3) += (*x2);
			f3 = obj->f(*x3);
			b3 -= b4; b3 *= (PHI-1); b3 += b2;
			if(fm > f3) fm = f3, bm = b3;
		}

		if(f2>f3){
			fnewbound = (b4-b1)/(b2-b1)*(f2-f1)+f1;
			if(fnewbound > fbound) fbound = fnewbound;

			xtmp = x1; x1 = x2; x2 = x3; x3 = xtmp;
			f1 = f2; f2 = f3;
			b1 = b2; b2 = b3;

			(*x3) = (*x1); (*x3) += (*x4); (*x3) -= (*x2);
			f3 = obj->f(*x3);
			b3 = b1 + b4 - b2;      

			if(fm > f3) fm = f3, bm = b3;
		}
		else{
			fnewbound = (b1-b4)/(b3-b4)*(f3-f4)+f4;
			if(fnewbound > fbound) fbound = fnewbound;
	    
			xtmp = x4; x4 = x3; x3 = x2; x2 = xtmp;
			f4 = f3; f3 = f2;
			b4 = b3; b3 = b2;

			(*x2) = (*x1); (*x2) += (*x4); (*x2) -= (*x3);
			f2 = obj->f(*x2);
			b2 = b1 + b4 - b3;

			if(fm > f2) fm = f2, bm = b2;
		}
		if(fabs(fbound-fm)/(fbound+fm) < 0.5*1e-9) break;
	}  

	delete x1; delete x2; delete x3; delete x4;
	return bm;
}


// golden search between A and B
Real section_search ( Vector &A, 
                      Vector &B, 
                      Function *obj, 
                      int iterations,
                      bool to_use_golden_ratio,
                      Real b1, Real b2)
{
	// Check whether the function can be reduced (a reducable function)
	// by casting it to ReducableFunction class
	ReducableFunction *casted_obj = dynamic_cast<ReducableFunction*> (obj);

	if(casted_obj != NULL){
		// If it can be casted --> it is a reducable function
		// then do the search with the reduced function and variables instead
		Function* reduced_obj = casted_obj->reduced_function();
		Vector A_ = casted_obj->reduced_variable(A);
		Vector B_ = casted_obj->reduced_variable(B);
		Real lambda = section_search( A_, B_,
		                              reduced_obj,
		                              iterations,
		                              to_use_golden_ratio,
		                              b1, b2);
		delete reduced_obj;
		return lambda;
	}

	if(to_use_golden_ratio) return golden_section_search(A,B,obj,iterations);
	return general_section_search (A,B,obj,iterations,b1,b2);
}

// Naive line search between A and B
Real line_search (Vector &A, Vector &B, Function *obj, int niteration){
	Vector x(A), dx(B);
	Real fmin = obj->f(x), imin = 0.0, f;
	dx -= A; dx *= (1.0/niteration);
	FOR(i, niteration){
		x += dx;
		f = obj->f(x);
		if(f < fmin) fmin = f, imin = i+1;
	}
	return imin / niteration;
}
