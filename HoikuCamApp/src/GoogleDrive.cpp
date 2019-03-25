#include "mbed.h"
#include <curl/curl.h>
#include <string.h>
#include "GoogleDrive.h"
#include "jsonsl.h"

#define SKIP_PEER_VERIFICATION
//#define SKIP_HOSTNAME_VERIFICATION

const char SCOPE_DRIVE_FILE[] = "https://www.googleapis.com/auth/drive.file";
const char GRANT_TYPE_DEVICE[] = "http://oauth.net/grant_type/device/1.0";

char response[80];
char errbuf[CURL_ERROR_SIZE];

int read_client_info(google_drive_t *gd, const char *fname);
void curl_setopt_common(CURL *curl);
int google_drive_error_callback(jsonsl_t jsn, jsonsl_error_t err,
	struct jsonsl_state_st *state, char *errat);
void google_drive_state_callback(jsonsl_t jsn, jsonsl_action_t action,
	struct jsonsl_state_st *state, const char *buf);

void client_init(google_drive_t *gd)
{
	memset(gd, 0, sizeof(google_drive_t));

	read_client_info(gd, ".\\client_secret.json");

	curl_global_init(CURL_GLOBAL_DEFAULT);
}

size_t write_callback(void *buffer, size_t size, size_t nmemb, void *arg)
{
	google_drive_t *gd = (google_drive_t *)arg;
	jsonsl_t jsn = gd->jsn;
	size_t len = size * nmemb, pos;
	jsonsl_char_t data;

	for (pos = 0; pos < len; pos++) {
		data = *((jsonsl_char_t *)&((uint8_t *)buffer)[pos]);

		if ((gd->jsn_buf_pos >= 0) && (gd->jsn_buf_pos < sizeof(gd->jsn_buf)))
			gd->jsn_buf[gd->jsn_buf_pos++] = data;
		jsonsl_feed(jsn, &data, 1);
	}

	return len;
}

void client_info_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
	switch (gd->state) {
	case gdpsRoot:
		if (state->level != 2) {
			break;
		}
		else if (strcmp(buf, "installed") == 0) {
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsInstalled:
		if (state->level == 2) {
			gd->state = gdpsRoot;
			break;
		}
		if (state->level != 3) {
			break;
		}
		else if (strcmp(buf, "client_id") == 0) {
			gd->state = gdpsClientId;
		}
		else if (strcmp(buf, "project_id") == 0) {
			gd->state = gdpsProjectId;
		}
		else if (strcmp(buf, "auth_uri") == 0) {
			gd->state = gdpsAuthUri;
		}
		else if (strcmp(buf, "token_uri") == 0) {
			gd->state = gdpsTokenUri;
		}
		else if (strcmp(buf, "auth_provider_x509_cert_url") == 0) {
			gd->state = gdpsAuthProviderX509CertUrl;
		}
		else if (strcmp(buf, "client_secret") == 0) {
			gd->state = gdpsClientSecret;
		}
		else if (strcmp(buf, "redirect_uris") == 0) {
			gd->state = gdpsRedirectUris;
			gd->index = 0;
		}
		break;
	case gdpsClientId:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.client_id, buf, sizeof(gd->client_info.client_id));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsProjectId:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.project_id, buf, sizeof(gd->client_info.project_id));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsAuthUri:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.auth_uri, buf, sizeof(gd->client_info.auth_uri));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsTokenUri:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.token_uri, buf, sizeof(gd->client_info.token_uri));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsAuthProviderX509CertUrl:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.auth_provider_x509_cert_url, buf, sizeof(gd->client_info.auth_provider_x509_cert_url));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsClientSecret:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.client_secret, buf, sizeof(gd->client_info.client_secret));
			gd->state = gdpsInstalled;
		}
		break;
	case gdpsRedirectUris:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->client_info.redirect_uris[gd->index], buf, sizeof(gd->client_info.redirect_uris[0]));
			gd->index++;
			if (gd->index == 2) {
				gd->state = gdpsInstalled;
			}
		}
		break;
	}
}

void client_info_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

int read_client_info(google_drive_t *gd, const char *fname)
{
	FILE *file;
	int ret;
	jsonsl_t jsn = gd->jsn = jsonsl_new(5);
	char buffer[16];
	size_t len, pos;
	jsonsl_char_t data;

	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = client_info_state_start;
	gd->end = client_info_state_end;
	gd->state = gdpsRoot;

	file = fopen(fname, "r");
	if (file == NULL) {
		printf("not open %s file %d\n", fname, ret);
		ret = -1;
		goto error;
	}

	for (;;) {
		len = fread(buffer, 1, sizeof(buffer), file);
		if (len <= 0)
			break;

		for (pos = 0; pos < len; pos++) {
			data = *((jsonsl_char_t *)&((uint8_t *)buffer)[pos]);

			if ((gd->jsn_buf_pos >= 0) && (gd->jsn_buf_pos < sizeof(gd->jsn_buf)))
				gd->jsn_buf[gd->jsn_buf_pos++] = data;
			jsonsl_feed(jsn, &data, 1);
		}
	}

	fclose(file);

	ret = 0;
error:
	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	return ret;
}

void get_device_id_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
	switch (gd->state) {
	case gdpsRoot:
		if (state->level != 2) {
			break;
		}
		else if (strcmp(buf, "device_code") == 0) {
			gd->state = gdpsDeviceCode;
		}
		else if (strcmp(buf, "user_code") == 0) {
			gd->state = gdpsUserCode;
		}
		else if (strcmp(buf, "expires_in") == 0) {
			gd->state = gdpsExpiresIn;
		}
		else if (strcmp(buf, "interval") == 0) {
			gd->state = gdpsInterval;
		}
		else if (strcmp(buf, "verification_url") == 0) {
			gd->state = gdpsVerificationUrl;
		}
		else if (strcmp(buf, "error") == 0) {
			gd->state = gdpsError;
		}
		else if (strcmp(buf, "error_description") == 0) {
			gd->state = gdpsErrorDescription;
		}
		break;
	case gdpsDeviceCode:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.device_code, buf, sizeof(gd->credential.device_code));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsUserCode:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.user_code, buf, sizeof(gd->credential.user_code));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsExpiresIn:
		if (state->type == JSONSL_T_SPECIAL) {
			gd->credential.expires_in = atoi(buf);
			gd->state = gdpsRoot;
		}
		break;
	case gdpsInterval:
		if (state->type == JSONSL_T_SPECIAL) {
			gd->credential.interval = atoi(buf);
			gd->state = gdpsRoot;
		}
		break;
	case gdpsVerificationUrl:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.verification_url, buf, sizeof(gd->credential.verification_url));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsError:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error, buf, sizeof(gd->error.error));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsErrorDescription:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error_description, buf, sizeof(gd->error.error_description));
			gd->state = gdpsRoot;
		}
		break;
	}
}

void get_device_id_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

int get_device_id(google_drive_t *gd, const char *scope)
{
	client_info_t *client_info = &gd->client_info;
	credential_t *credential = &gd->credential;
	CURLcode ret;
	CURL *curl;
	struct curl_slist *slist1;
	size_t len;
	char *postdata;

	len = strnlen(client_info->client_id, sizeof(client_info->client_id));
	if (len == 0)
		return -1;

	jsonsl_t jsn = gd->jsn = jsonsl_new(3);
	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = get_device_id_state_start;
	gd->end = get_device_id_state_end;
	gd->state = gdpsRoot;

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type: application/x-www-form-urlencoded");

	curl = curl_easy_init();

	char *client_id = curl_easy_escape(curl, client_info->client_id, len);
	char *esc_scope = curl_easy_escape(curl, scope, strlen(scope));

	len = sizeof("client_id=") + strlen(client_id) + sizeof("scope=") + strlen(esc_scope);
	postdata = (char *)malloc(len);
	snprintf(postdata, len, "client_id=%s&scope=%s", client_id, esc_scope);

	curl_free(client_id);
	curl_free(esc_scope);

	curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.google.com/o/oauth2/device/code");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strnlen(postdata, len));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
	curl_setopt_common(curl);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, gd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	ret = curl_easy_perform(curl);

	free(postdata);

	curl_easy_cleanup(curl);
	curl = NULL;
	curl_slist_free_all(slist1);
	slist1 = NULL;

	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	return (int)ret;
}

void get_access_token_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
	switch (gd->state) {
	case gdpsRoot:
		if (state->level != 2) {
			break;
		}
		else if (strcmp(buf, "access_token") == 0) {
			gd->state = gdpsAccessToken;
		}
		else if (strcmp(buf, "expires_in") == 0) {
			gd->state = gdpsExpiresIn;
		}
		else if (strcmp(buf, "refresh_token") == 0) {
			gd->state = gdpsRefreshToken;
		}
		else if (strcmp(buf, "scope") == 0) {
			gd->state = gdpsScope;
		}
		else if (strcmp(buf, "token_type") == 0) {
			gd->state = gdpsTokenType;
		}
		else if (strcmp(buf, "error") == 0) {
			gd->state = gdpsError;
		}
		else if (strcmp(buf, "error_description") == 0) {
			gd->state = gdpsErrorDescription;
		}
		break;
	case gdpsAccessToken:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.access_token, buf, sizeof(gd->credential.access_token));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsExpiresIn:
		if (state->type == JSONSL_T_SPECIAL) {
			gd->credential.expires_in = atoi(buf);
			gd->state = gdpsRoot;
		}
		break;
	case gdpsRefreshToken:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.refresh_token, buf, sizeof(gd->credential.refresh_token));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsScope:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.scope, buf, sizeof(gd->credential.scope));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsTokenType:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.token_type, buf, sizeof(gd->credential.token_type));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsError:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error, buf, sizeof(gd->error.error));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsErrorDescription:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error_description, buf, sizeof(gd->error.error_description));
			gd->state = gdpsRoot;
		}
		break;
	}
}

void get_access_token_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

int get_access_token(google_drive_t *gd)
{
	client_info_t *client_info = &gd->client_info;
	credential_t *credential = &gd->credential;
	CURLcode ret;
	CURL *curl;
	struct curl_slist *slist1;
	size_t len;
	char *postdata;

	len = strlen(credential->device_code);
	if (len == 0)
		return -1;

	jsonsl_t jsn = gd->jsn = jsonsl_new(3);
	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = get_access_token_state_start;
	gd->end = get_access_token_state_end;
	gd->state = gdpsRoot;

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type: application/x-www-form-urlencoded");

	curl = curl_easy_init();

	char *client_id = curl_easy_escape(curl, client_info->client_id, strnlen(client_info->client_id, sizeof(client_info->client_id)));
	char *client_secret = curl_easy_escape(curl, client_info->client_secret, strnlen(client_info->client_secret, sizeof(client_info->client_secret)));
	char *grant_type = curl_easy_escape(curl, GRANT_TYPE_DEVICE, strnlen(GRANT_TYPE_DEVICE, sizeof(GRANT_TYPE_DEVICE)));

	len = sizeof("client_id=") + strlen(client_id) + sizeof("client_secret=") + strlen(client_secret)
		+ sizeof("code=") + strlen(credential->device_code) + sizeof("grant_type=") + strlen(grant_type);
	postdata = (char *)malloc(len);
	snprintf(postdata, len, "client_id=%s&client_secret=%s&code=%s&grant_type=%s",
		client_id, client_secret, credential->device_code, grant_type);

	curl_free(client_id);
	curl_free(client_secret);
	curl_free(grant_type);

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/oauth2/v4/token");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strnlen(postdata, len));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
	curl_setopt_common(curl);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, gd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	ret = curl_easy_perform(curl);

	free(postdata);

	curl_easy_cleanup(curl);
	curl = NULL;
	curl_slist_free_all(slist1);
	slist1 = NULL;

	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	return (int)ret;
}

void update_access_token_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
	switch (gd->state) {
	case gdpsRoot:
		if (state->level != 2) {
			break;
		}
		else if (strcmp(buf, "access_token") == 0) {
			gd->state = gdpsAccessToken;
		}
		else if (strcmp(buf, "expires_in") == 0) {
			gd->state = gdpsExpiresIn;
		}
		else if (strcmp(buf, "scope") == 0) {
			gd->state = gdpsScope;
		}
		else if (strcmp(buf, "token_type") == 0) {
			gd->state = gdpsTokenType;
		}
		else if (strcmp(buf, "error") == 0) {
			gd->state = gdpsError;
		}
		else if (strcmp(buf, "error_description") == 0) {
			gd->state = gdpsErrorDescription;
		}
		break;
	case gdpsAccessToken:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.access_token, buf, sizeof(gd->credential.access_token));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsExpiresIn:
		if (state->type == JSONSL_T_SPECIAL) {
			gd->credential.expires_in = atoi(buf);
			gd->state = gdpsRoot;
		}
		break;
	case gdpsScope:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.scope, buf, sizeof(gd->credential.scope));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsTokenType:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->credential.token_type, buf, sizeof(gd->credential.token_type));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsError:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error, buf, sizeof(gd->error.error));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsErrorDescription:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error_description, buf, sizeof(gd->error.error_description));
			gd->state = gdpsRoot;
		}
		break;
	}
}

void update_access_token_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

int update_access_token(google_drive_t *gd)
{
	client_info_t *client_info = &gd->client_info;
	credential_t *credential = &gd->credential;
	CURLcode ret;
	CURL *curl;
	struct curl_slist *slist1;
	size_t len;
	char *postdata;

	len = strlen(credential->refresh_token);
	if (len == 0)
		return -1;

	jsonsl_t jsn = gd->jsn = jsonsl_new(3);
	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = update_access_token_state_start;
	gd->end = update_access_token_state_end;
	gd->state = gdpsRoot;

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type: application/x-www-form-urlencoded");

	curl = curl_easy_init();

	char *client_id = curl_easy_escape(curl, client_info->client_id, strnlen(client_info->client_id, sizeof(client_info->client_id)));
	char *client_secret = curl_easy_escape(curl, client_info->client_secret, strnlen(client_info->client_secret, sizeof(client_info->client_secret)));

	len = sizeof("client_id=") + strlen(client_id) + sizeof("client_secret=") + strlen(client_secret)
		+ sizeof("refresh_token=") + strlen(credential->refresh_token);
	postdata = (char *)malloc(len);
	snprintf(postdata, len, "client_id=%s&client_secret=%s&code=%s&grant_type=refresh_token",
		client_id, client_secret, credential->refresh_token);

	curl_free(client_id);
	curl_free(client_secret);

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/oauth2/v4/token");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strnlen(postdata, len));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt_common(curl);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, gd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	ret = curl_easy_perform(curl);

	free(postdata);

	curl_easy_cleanup(curl);
	curl = NULL;
	curl_slist_free_all(slist1);
	slist1 = NULL;

	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	return (int)ret;
}

void revoke_device_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

void revoke_device_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

int revoke_device(google_drive_t *gd)
{
	client_info_t *client_info = &gd->client_info;
	credential_t *credential = &gd->credential;
	CURLcode ret;
	CURL *curl;
	size_t len;
	struct curl_slist *slist1;
	char *postdata;

	len = strlen(credential->access_token);
	if (len == 0)
		return -1;

	jsonsl_t jsn = gd->jsn = jsonsl_new(3);
	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = revoke_device_state_start;
	gd->end = revoke_device_state_end;
	gd->state = gdpsRoot;

	slist1 = NULL;
	slist1 = curl_slist_append(slist1, "Content-Type: application/x-www-form-urlencoded");

	curl = curl_easy_init();

	len = sizeof("token=") + strlen(credential->access_token);
	postdata = (char *)malloc(len);
	snprintf(postdata, len, "token=%s", credential->access_token);

	curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.google.com/o/oauth2/revoke");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strnlen(postdata, len));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
	curl_setopt_common(curl);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, gd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	ret = curl_easy_perform(curl);

	free(postdata);

	curl_easy_cleanup(curl);
	curl = NULL;
	curl_slist_free_all(slist1);
	slist1 = NULL;

	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	memset(credential, 0, sizeof(*credential));

	return (int)ret;
}

void upload_file_state_start(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
	switch (gd->state) {
	case gdpsRoot:
		if (state->level != 2) {
			break;
		}
		else if (strcmp(buf, "kind") == 0) {
			gd->state = gdpsKind;
		}
		else if (strcmp(buf, "id") == 0) {
			gd->state = gdpsId;
		}
		else if (strcmp(buf, "name") == 0) {
			gd->state = gdpsName;
		}
		else if (strcmp(buf, "mimeType") == 0) {
			gd->state = gdpsMimeType;
		}
		else if (strcmp(buf, "error") == 0) {
			gd->state = gdpsError;
		}
		else if (strcmp(buf, "error_description") == 0) {
			gd->state = gdpsErrorDescription;
		}
		break;
	case gdpsKind:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->file.kind, buf, sizeof(gd->file.kind));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsId:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->file.id, buf, sizeof(gd->file.id));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsName:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->file.name, buf, sizeof(gd->file.name));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsMimeType:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->file.mimeType, buf, sizeof(gd->file.mimeType));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsError:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error, buf, sizeof(gd->error.error));
			gd->state = gdpsRoot;
		}
		break;
	case gdpsErrorDescription:
		if (state->type == JSONSL_T_STRING) {
			strlcpy(gd->error.error_description, buf, sizeof(gd->error.error_description));
			gd->state = gdpsRoot;
		}
		break;
	}
}

void upload_file_state_end(google_drive_t *gd, struct jsonsl_state_st *state, const char *buf)
{
}

size_t read_callback(char *buffer, size_t size, size_t nitems, void *arg)
{
	FILE *file = (FILE *)arg;
	size_t ret = 0;
	int res;

	ret = fread(buffer, size, nitems, file);
	if (ret < 0)
		return 0;

	int rest = ret;
	int len;

	while (rest > 0) {
		len = rest;
		if (len > (sizeof(response) - 1)) {
			len = sizeof(response) - 1;
		}

		memcpy(response, buffer, len);

		response[len] = '\0';

		printf(response);

		ThisThread::sleep_for(100);

		rest -= len;
		buffer = (char *)buffer + len;
	}

	return ret;
}

int seek_callback(void *arg, curl_off_t offset, int origin)
{
	FILE *file = (FILE *)arg;
	int ret;

	ret = fseek(file, offset, origin);
	if (ret < 0)
		return CURL_SEEKFUNC_FAIL;

	return CURL_SEEKFUNC_OK;
}

int upload_file(google_drive_t *gd, const char *filename, const char *localfilepath)
{
	int ret = 0;
	CURL *curl;
	curl_mime *mime;
	curl_mimepart *part;
	struct curl_slist *slist1;
	size_t len;
	char *postdata, *authorization;
	static const char buf[] = "Expect:";
	FILE *file;
	struct stat stat;

	len = strnlen(gd->credential.access_token, sizeof(gd->credential.access_token));
	if (len <= 0)
		return -1;

	jsonsl_t jsn = gd->jsn = jsonsl_new(3);
	jsn->data = gd;
	jsn->error_callback = google_drive_error_callback;
	jsn->action_callback = google_drive_state_callback;
	jsonsl_enable_all_callbacks(jsn);

	gd->jsn_buf_pos = -1;
	gd->start = upload_file_state_start;
	gd->end = upload_file_state_end;
	gd->state = gdpsRoot;

	ret = ::stat(localfilepath, &stat);
	if (ret != 0)
		return -1;

	file = fopen(localfilepath, "r");
	if (file == NULL) {
		printf("log file open error %d\n", errno);
		ret = -1;
		goto error;
	}

	len = sizeof("{\"name\":\"\"}") + strlen(filename);
	postdata = (char *)malloc(len);
	snprintf(postdata, len, "{\"name\":\"%s\"}", filename);

	curl = curl_easy_init();

	mime = curl_mime_init(curl);
	part = curl_mime_addpart(mime);
	curl_mime_name(part, "metadata");
	curl_mime_type(part, "application/json;charset=utf-8");
	curl_mime_data(part, postdata, CURL_ZERO_TERMINATED);
	free(postdata);

	part = curl_mime_addpart(mime);
	curl_mime_name(part, "file");
	curl_mime_type(part, "application/json");
	curl_mime_data_cb(part, stat.st_size, read_callback, seek_callback, NULL, file);

	len = sizeof("Authorization: Bearer ") + strnlen(gd->credential.access_token, sizeof(gd->credential.access_token));
	authorization = (char *)malloc(len);
	snprintf(authorization, len, "Authorization: Bearer %s", gd->credential.access_token);
	slist1 = NULL;
	slist1 = curl_slist_append(slist1, authorization);
	free(authorization);

	slist1 = curl_slist_append(slist1, buf);

	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt_common(curl);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, gd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	if (curl_easy_perform(curl) != CURLE_OK)
		ret = -1;

	curl_easy_cleanup(curl);
	curl = NULL;
	curl_mime_free(mime);
	mime = NULL;
	curl_slist_free_all(slist1);
	slist1 = NULL;

	fclose(file);

error:
	gd->jsn = NULL;
	jsonsl_destroy(jsn);

	return (int)ret;
}

int google_drive_error_callback(jsonsl_t jsn, jsonsl_error_t err,
	struct jsonsl_state_st *state, char *errat)
{
	return 0;
}

void google_drive_state_callback(jsonsl_t jsn, jsonsl_action_t action,
	struct jsonsl_state_st *state, const char *buf)
{
	google_drive_t *gd = (google_drive_t *)jsn->data;

	switch (action) {
	case JSONSL_ACTION_PUSH:
		switch (state->type) {
		case JSONSL_T_SPECIAL:
			gd->jsn_buf[0] = *buf;
			gd->jsn_buf_pos = 1;
			break;
		case JSONSL_T_STRING:
		case JSONSL_T_HKEY:
			gd->jsn_buf_pos = 0;
			break;
		default:
			gd->jsn_buf_pos = -1;
		}
		break;
	case JSONSL_ACTION_POP:
		switch (state->type) {
		case JSONSL_T_SPECIAL:
		case JSONSL_T_STRING:
		case JSONSL_T_HKEY:
			gd->jsn_buf_pos--;
			if (gd->jsn_buf_pos < sizeof(gd->jsn_buf)) {
				gd->jsn_buf[gd->jsn_buf_pos] = '\0';
			}
			gd->start(gd, state, gd->jsn_buf);
			break;
		default:
			gd->jsn_buf[0] = '\0';
			break;
		}
		gd->jsn_buf_pos = -1;
		break;
	default:
		gd->jsn_buf_pos = -1;
		break;
	}
}

void curl_setopt_common(CURL *curl)
{
	CURLcode res;

	/* ask libcurl to show us the verbose output */
	res = curl_easy_setopt(curl, CURLOPT_VERBOSE, true);
	if (res != CURLE_OK)
		printf("CURLOPT_VERBOSE failed: %s\n",
			curl_easy_strerror(res));

	/* set the error buffer as empty before performing a request */
	errbuf[0] = 0;

	/* provide a buffer to store errors in */
	res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	if (res != CURLE_OK)
		printf("CURLOPT_ERRORBUFFER failed: %s\n",
			curl_easy_strerror(res));

#ifdef SKIP_PEER_VERIFICATION
	/*
	* If you want to connect to a site who isn't using a certificate that is
	* signed by one of the certs in the CA bundle you have, you can skip the
	* verification of the server's certificate. This makes the connection
	* A LOT LESS SECURE.
	*
	* If you have a CA cert for the server stored someplace else than in the
	* default bundle, then the CURLOPT_CAPATH option might come handy for
	* you.
	*/
	res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	if (res != CURLE_OK)
		printf("CURLOPT_SSL_VERIFYPEER failed: %s\n",
			curl_easy_strerror(res));
#else
	res = curl_easy_setopt(curl, CURLOPT_CAINFO, "0:/certs/ca-cert.pem");
	if (res != CURLE_OK)
		printf("CURLOPT_CAINFO failed: %s\n",
			curl_easy_strerror(res));

	res = curl_easy_setopt(curl, CURLOPT_SSLCERT, "0:/certs/client-cert.pem");
	if (res != CURLE_OK)
		printf("CURLOPT_SSLCERT failed: %s\n",
			curl_easy_strerror(res));

	res = curl_easy_setopt(curl, CURLOPT_SSLKEY, "0:/certs/client-key.pem");
	if (res != CURLE_OK)
		printf("CURLOPT_SSLKEY failed: %s\n",
			curl_easy_strerror(res));
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
	/*
	* If the site you're connecting to uses a different host name that what
	* they have mentioned in their server certificate's commonName (or
	* subjectAltName) fields, libcurl will refuse to connect. You can skip
	* this check, but this will make the connection less secure.
	*/
	res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	if (res != CURLE_OK)
		printf("CURLOPT_SSL_VERIFYHOST failed: %s\n",
			curl_easy_strerror(res));
#endif

	/*res = curl_easy_setopt(curl, CURLOPT_PROXY, "https://proxy.example.com:8080");
	if (res != CURLE_OK)
		printf("CURLOPT_PROXY failed: %s\n",
			curl_easy_strerror(res));*/

	res = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	if (res != CURLE_OK)
		printf("CURLOPT_NOPROGRESS failed: %s\n",
			curl_easy_strerror(res));

	res = curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	if (res != CURLE_OK)
		printf("CURLOPT_MAXREDIRS failed: %s\n",
			curl_easy_strerror(res));

	res = curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	if (res != CURLE_OK)
		printf("CURLOPT_TCP_KEEPALIVE failed: %s\n",
			curl_easy_strerror(res));
}

void client_final(void)
{
	curl_global_cleanup();
}

GoogleDriveTask::GoogleDriveTask(TaskThread *taskThread) :
	_state(State::Idle),
	_taskThread(taskThread),
	_request(0),
	_retry(0)
{
}

GoogleDriveTask::~GoogleDriveTask()
{
}

void GoogleDriveTask::OnStart()
{
	client_init(&gd);
}

void GoogleDriveTask::OnEnd()
{
	client_final();
}

void GoogleDriveTask::ProcessEvent(InterTaskSignals::T signals)
{
}

int GoogleDriveTask::CheckRequest()
{
	int request = _request;

	if (request == 0)
		return 0;

	_request = 0;

	return request;
}

void GoogleDriveTask::Process()
{
	int ret;

	if (_timer != 0)
		return;

	switch (_state) {
	case State::Idle:
		ret = CheckRequest();
		switch (ret) {
		case 1:
			_state = State::WantDeviceId;
			_timer = 0;
			break;
		case 2:
			_state = State::Revoke;
			_timer = 0;
			break;
		}
		break;
	case State::WantDeviceId:
		ret = get_device_id(&gd, SCOPE_DRIVE_FILE);
		if (ret == 0) {
			printf("Enter the code at the following URL\n");
			printf("url:  %s\n", gd.credential.verification_url);
			printf("code: %s\n", gd.credential.user_code);

			_retry = gd.credential.expires_in / gd.credential.interval;
			_state = State::WantAccessToken;
			_timer = gd.credential.interval;
		}
		else {
			_state = State::Idle;
			_timer = -1;
		}
		break;
	case State::WantAccessToken:
		ret = get_access_token(&gd);
		if (ret == 0) {
			_retry = 3;
			_state = State::UpdateAccessToken;
			_timer = (gd.credential.expires_in - 2) * 1000;
		}
		else if (--_retry > 0) {
			_state = State::WantAccessToken;
			_timer = 1000;
		}
		else {
			_state = State::Idle;
			_timer = -1;
		}
		break;
	case State::UpdateAccessToken:
		ret = update_access_token(&gd);
		if (ret == 0) {
			_retry = 3;
			_state = State::UpdateAccessToken;
			_timer = (gd.credential.expires_in - 2) * 1000;
		}
		else if (--_retry > 0) {
			_state = State::UpdateAccessToken;
			_timer = gd.credential.interval;
		}
		else {
			_state = State::Idle;
			_timer = -1;
		}
		break;
	case State::Revoke:
		ret = revoke_device(&gd);
		if (ret == 0) {
			_state = State::Idle;
			_timer = -1;
		}
		else {
			_state = State::Idle;
			_timer = -1;
		}
		break;
	}
}
