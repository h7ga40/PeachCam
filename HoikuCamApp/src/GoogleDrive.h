#ifndef _GOOGLEDRIVE_H_
#define _GOOGLEDRIVE_H_

#include "TaskBase.h"

typedef struct client_info_t {
	char client_id[80];
	char project_id[32];
	char auth_uri[64];
	char token_uri[48];
	char auth_provider_x509_cert_url[48];
	char client_secret[32];
	char redirect_uris[2][32];
} client_info_t;

typedef struct credential_t {
	char device_code[100];
	char user_code[12];
	int expires_in;
	int interval;
	char verification_url[32];
	char access_token[132];
	char refresh_token[64];
	char scope[64];
	char token_type[16];
} credential_t;

typedef struct error_response_t {
	char error[64];
	char error_description[64];
} error_response_t;

typedef struct drive_file_t {
	char kind[32];
	char id[36];
	char name[256];
	char mimeType[40];
} drive_file_t;

typedef enum google_drive_parse_state_t {
	gdpsRoot,
	gdpsInstalled,
	gdpsClientId,
	gdpsProjectId,
	gdpsAuthUri,
	gdpsTokenUri,
	gdpsAuthProviderX509CertUrl,
	gdpsClientSecret,
	gdpsRedirectUris,
	gdpsDeviceCode,
	gdpsUserCode,
	gdpsExpiresIn,
	gdpsInterval,
	gdpsVerificationUrl,
	gdpsAccessToken,
	gdpsRefreshToken,
	gdpsScope,
	gdpsTokenType,
	gdpsKind,
	gdpsId,
	gdpsName,
	gdpsMimeType,
	gdpsError,
	gdpsErrorDescription,
} google_drive_parse_state_t;

struct jsonsl_st;
typedef struct jsonsl_st *jsonsl_t;

typedef void (*parser_callback_t)(struct google_drive_t *gd, struct jsonsl_state_st *state, const char *buf);

typedef struct google_drive_t {
	client_info_t client_info;
	credential_t credential;
	drive_file_t file;
	error_response_t error;
	jsonsl_t jsn;
	google_drive_parse_state_t state;
	int index;
	char jsn_buf[256];
	int jsn_buf_pos;
	parser_callback_t start;
	parser_callback_t end;
} google_drive_t;

class GoogleDriveTask : public Task
{
public:
	class State
	{
	public:
		enum T {
			Idle,
			WantDeviceId,
			Revoke,
			WantAccessToken,
			UpdateAccessToken,
		};
	};
public:
	GoogleDriveTask(TaskThread *taskThread);
	virtual ~GoogleDriveTask();
private:
	State::T _state;
	TaskThread *_taskThread;
	google_drive_t gd;
	int _request;
	int _retry;
	int CheckRequest();
public:
	State::T GetState() { return _state; }
	void OnStart() override;
	void OnEnd() override;
	void ProcessEvent(InterTaskSignals::T signals) override;
	void Process() override;
};

#endif /* _GOOGLEDRIVE_H_ */
