#ifndef DEMOCOMPONENT_H
#define DEMOCOMPONENT_H

namespace icinga
{

class DemoComponent : public IcingaComponent
{
private:
	Timer::Ptr m_DemoTimer;
	VirtualEndpoint::Ptr m_DemoEndpoint;

	int DemoTimerHandler(const TimerEventArgs& tea);
	int NewEndpointHandler(const NewEndpointEventArgs& neea);
	int HelloWorldRequestHandler(const NewRequestEventArgs& nrea);

public:
	virtual string GetName(void) const;
	virtual void Start(void);
	virtual void Stop(void);
};

}

#endif /* DEMOCOMPONENT_H */
